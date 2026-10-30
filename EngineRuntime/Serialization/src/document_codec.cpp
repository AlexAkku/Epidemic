#include "Epidemic/Runtime/Serialization/document_codec.h"
#include "archive_tree.h"
#include <algorithm>
#include <bit>
#include <limits>
#include <exception>

namespace epidemic::runtime {
class DocumentCodecAccess {
  public:
    static const SerializedDocument::Impl &Get(const SerializedDocument &value) {
        return *value.impl_;
    }

    static SerializedDocument Make(foundation::StringId type, SchemaVersion schema,
                                   ArchiveObjectPtr root) {
        auto value = std::make_shared<SerializedDocument::Impl>();
        value->type_id = type;
        value->schema_version = schema;
        value->root = std::move(root);
        return SerializedDocument{std::move(value)};

    }

};

namespace {
struct Invalid : std::exception {};

enum class Width : unsigned { Byte = 1, Word = 4, Wide = 8 };

constexpr std::size_t kMaxValues = 262144, kMaxDepth = 128;

void Require(bool value) {
    if (!value)
        throw Invalid{};

}

std::uint64_t Hash(std::span<const std::byte> bytes) {
    std::uint64_t hash = 14695981039346656037ull;
    for (const auto value : bytes) {
        hash ^= std::to_integer<unsigned char>(value);
        hash *= 1099511628211ull;
    }

    return hash;
}

struct Output {
    std::vector<std::byte> bytes;
    std::size_t values = 0;

    void Number(std::uint64_t value, Width size = Width::Wide) {
        const auto width = static_cast<unsigned>(size);
        Require(width <= kMaxDocumentBytes - bytes.size());
        for (unsigned i = 0; i < width; ++i)
            bytes.push_back(static_cast<std::byte>((value >> (i * 8u)) & 255ull));
    }

    void Raw(std::span<const std::byte> value) {
        Require(value.size() <= kMaxDocumentBytes - bytes.size());
        bytes.insert(bytes.end(), value.begin(), value.end());
    }

    void String(std::string_view value) {
        Number(value.size(), Width::Word);
        Raw(std::as_bytes(std::span{value.data(), value.size()}));
    }

    void Value(const ArchiveValue &value, std::size_t depth) {
        Require(depth <= kMaxDepth && ++values <= kMaxValues);
        Number(static_cast<unsigned>(value.Kind()), Width::Byte);
        std::visit(
            [&](const auto &data) {
                using T = std::decay_t<decltype(data)>;
                if constexpr (std::is_same_v<T, std::string>)
                    String(data);
                else if constexpr (std::is_same_v<T, std::uint64_t>)
                    Number(data);
                else if constexpr (std::is_same_v<T, std::int64_t> || std::is_same_v<T, double>)
                    Number(std::bit_cast<std::uint64_t>(data));
                else if constexpr (std::is_same_v<T, bool>)
                    Number(data ? 1 : 0, Width::Byte);
                else if constexpr (std::is_same_v<T, std::vector<std::byte>>) {
                    Number(data.size(), Width::Word);
                    Raw(data);
                } else if constexpr (std::is_same_v<T, ArchiveObjectPtr>) {
                    Require(data != nullptr && data->fields.size() <= kMaxValues - values);
                    std::vector<std::string_view> keys;
                    keys.reserve(data->fields.size());
                    for (const auto &[key, ignored] : data->fields) {
                        (void)ignored;
                        keys.push_back(key);
                    }

                    std::sort(keys.begin(), keys.end());
                    Number(keys.size(), Width::Word);
                    for (const auto key : keys) {
                        String(key);
                        Value(data->fields.at(std::string(key)), depth + 1);
                    }

                } else if constexpr (std::is_same_v<T, ArchiveArrayPtr>) {
                    Require(data != nullptr && data->elements.size() <= kMaxValues - values &&
                            data->initialized.size() == data->elements.size() &&
                            std::all_of(data->initialized.begin(), data->initialized.end(),
                                        [](bool item) { return item; }));
                    Number(data->elements.size(), Width::Word);
                    for (const auto &item : data->elements)
                        Value(item, depth + 1);
                }

            },
            value.storage);
    }

};

struct Input {
    std::span<const std::byte> bytes;
    std::size_t at = 0, values = 0;

    std::uint64_t Number(Width size = Width::Wide) {
        const auto width = static_cast<unsigned>(size);
        Require(width <= bytes.size() - at);
        std::uint64_t value = 0;
        for (unsigned i = 0; i < width; ++i)
            value |= std::uint64_t{std::to_integer<unsigned char>(bytes[at++])} << (i * 8u);
        return value;
    }

    std::span<const std::byte> Raw() {
        const auto count = Number(Width::Word);
        Require(count <= bytes.size() - at);
        const auto view = bytes.subspan(at, static_cast<std::size_t>(count));
        at += view.size();
        return view;
    }

    std::string String() {
        const auto view = Raw();
        return {reinterpret_cast<const char *>(view.data()), view.size()};

    }

    ArchiveValue Value(std::size_t depth) {
        Require(depth <= kMaxDepth && ++values <= kMaxValues);
        const auto tag = Number(Width::Byte);
        switch (tag) {
        case 0:
            return {};

        case 1:
            return {String()};

        case 2:
            return {Number()};

        case 3:
            return {std::bit_cast<std::int64_t>(Number())};

        case 4:
            return {std::bit_cast<double>(Number())};

        case 5: {
            const auto value = Number(Width::Byte);
            Require(value <= 1);
            return {value != 0};

        }

        case 6: {
            const auto view = Raw();
            return {std::vector<std::byte>{view.begin(), view.end()}};

        }

        case 7: {
            const auto count = Number(Width::Word);
            Require(count <= kMaxValues - values && count <= (bytes.size() - at) / 5);
            auto object = std::make_shared<ArchiveObject>();
            std::string previous;
            for (std::uint64_t i = 0; i < count; ++i) {
                auto key = String();
                Require(i == 0 || previous < key);
                previous = key;
                auto value = Value(depth + 1);
                object->fields.emplace(std::move(key), std::move(value));
            }

            return {std::move(object)};

        }

        case 8: {
            const auto count = Number(Width::Word);
            Require(count <= kMaxValues - values && count <= bytes.size() - at);
            auto array = std::make_shared<ArchiveArray>();
            array->elements.reserve(static_cast<std::size_t>(count));
            for (std::uint64_t i = 0; i < count; ++i)
                array->elements.push_back(Value(depth + 1));
            array->initialized.assign(static_cast<std::size_t>(count), true);
            return {std::move(array)};

        }

        default:
            throw Invalid{};

        }

    }

};

foundation::Error Error() {
    return foundation::Error::Create("serialization.invalid_wire",
                                     "invalid or oversized archive encoding");
}

constexpr std::uint64_t kMagic = 0x31435241495045ull;
} // namespace

foundation::Result<std::vector<std::byte>> EncodeDocument(const SerializedDocument &document) {
    using Result = foundation::Result<std::vector<std::byte>>;
    try {
        Require(document.IsValid() && document.GetFormatVersion() == 1);
        const auto &data = DocumentCodecAccess::Get(document);
        Output output;
        output.Number(kMagic);
        output.Number(1, Width::Word);
        output.Number(data.type_id.Raw());
        output.Number(data.schema_version.major, Width::Word);
        output.Number(data.schema_version.minor, Width::Word);
        output.Number(data.schema_version.patch, Width::Word);
        output.Value(ArchiveValue{data.root}, 0);
        const auto hash = Hash(output.bytes);
        output.Number(hash);
        return Result::Success(std::move(output.bytes));
    } catch (const Invalid &) {
        return Result::Failure(Error());
    }

}

foundation::Result<SerializedDocument> DecodeDocument(std::span<const std::byte> bytes) {
    using Result = foundation::Result<SerializedDocument>;
    try {
        Require(bytes.size() >= 45 && bytes.size() <= kMaxDocumentBytes);
        Input checksum{bytes.last(8)};

        Require(checksum.Number() == Hash(bytes.first(bytes.size() - 8)));
        Input input{bytes.first(bytes.size() - 8)};

        Require(input.Number() == kMagic && input.Number(Width::Word) == 1);
        const auto type = foundation::StringId{input.Number()};

        const SchemaVersion schema{static_cast<std::uint32_t>(input.Number(Width::Word)),
                                   static_cast<std::uint32_t>(input.Number(Width::Word)),
                                   static_cast<std::uint32_t>(input.Number(Width::Word))};

        Require(type.IsValid() && schema != SchemaVersion{});
        auto root = input.Value(0);
        Require(input.at == input.bytes.size() && root.Kind() == ArchiveValueKind::Object);
        return Result::Success(
            DocumentCodecAccess::Make(type, schema,
                                      std::get<ArchiveObjectPtr>(std::move(root.storage))));
    } catch (const Invalid &) {
        return Result::Failure(Error());
    }

}

} // namespace epidemic::runtime
