#include "Epidemic/Runtime/Serialization/document_codec.h"
#include "Epidemic/Runtime/Serialization/serialization_services.h"
#include <algorithm>
#include <cstdio>
#include <limits>
#include <stdexcept>
using namespace epidemic;
using namespace epidemic::runtime;
namespace {
void Check(bool value) {
  if (!value)
    throw std::runtime_error("wire codec regression");
}
template <class T> T Take(foundation::Result<T> result) {
  Check(result.HasValue());
  return std::move(result).Value();
}
void Take(const foundation::Result<void> &result) { Check(result.HasValue()); }
void Checksum(std::vector<std::byte> &bytes) {
  std::uint64_t hash = 14695981039346656037ull;
  for (std::size_t i = 0; i < bytes.size() - 8; ++i) {
    hash ^= std::to_integer<unsigned char>(bytes[i]);
    hash *= 1099511628211ull;
  }
  for (unsigned i = 0; i < 8; ++i)
    bytes[bytes.size() - 8 + i] =
        static_cast<std::byte>((hash >> (i * 8u)) & 255ull);
}
void Test() {
  auto services = Take(CreateSerializationServices());
  auto writer = services.archives->CreateWriter();
  Take(writer->WriteUInt64("unsigned",
                           std::numeric_limits<std::uint64_t>::max()));
  Take(writer->WriteInt64("signed", std::numeric_limits<std::int64_t>::min()));
  Take(writer->WriteDouble("double", -1.25));
  Take(writer->WriteBool("bool", true));
  Take(writer->WriteNull("null"));
  Take(writer->WriteString("string", "тест"));
  const std::byte raw[]{std::byte{0}, std::byte{255}};
  Take(writer->WriteBytes("bytes", raw));
  Take(writer->BeginArray("array", 1));
  Take(writer->BeginArrayElement(0));
  Take(writer->WriteString("value", "nested"));
  Take(writer->EndArrayElement());
  Take(writer->EndArray());
  const auto type = foundation::StringId::FromString("codec.test");
  const auto document = Take(writer->Finalize(type, {3, 2, 1}));
  auto bytes = Take(EncodeDocument(document));
  auto restored = Take(DecodeDocument(bytes));
  Check(restored.GetTypeId() == type &&
        restored.GetSchemaVersion() == SchemaVersion{3, 2, 1} &&
        Take(EncodeDocument(restored)) == bytes);
  auto reader = Take(services.archives->CreateReader(restored));
  Check(Take(reader->ReadUInt64("unsigned")) ==
        std::numeric_limits<std::uint64_t>::max());
  Check(Take(reader->ReadInt64("signed")) ==
        std::numeric_limits<std::int64_t>::min());
  Check(Take(reader->ReadDouble("double")) == -1.25 &&
        Take(reader->ReadBool("bool")) && Take(reader->IsNull("null")));
  Check(Take(reader->ReadString("string")) == "тест" &&
        Take(reader->ReadBytes("bytes")) ==
            std::vector<std::byte>(std::begin(raw), std::end(raw)));
  Check(Take(reader->BeginArray("array")) == 1);
  Take(reader->BeginArrayElement(0));
  Check(Take(reader->ReadString("value")) == "nested");
  Take(reader->EndArrayElement());
  Take(reader->EndArray());
  auto invalid = bytes;
  invalid[8] = std::byte{2};
  Checksum(invalid);
  Check(!DecodeDocument(invalid));
  invalid = bytes;
  std::fill(invalid.begin() + 20, invalid.begin() + 32, std::byte{});
  Checksum(invalid);
  Check(!DecodeDocument(invalid));
  invalid = bytes;
  invalid.insert(invalid.end() - 8, std::byte{});
  Checksum(invalid);
  Check(!DecodeDocument(invalid));
  bytes[25] ^= std::byte{1};
  Check(!DecodeDocument(bytes));
  bytes.pop_back();
  Check(!DecodeDocument(bytes));
  Check(!EncodeDocument({}) && !DecodeDocument({}));
  std::vector<std::byte> oversized(kMaxDocumentBytes + 1);
  Check(!DecodeDocument(oversized));
}
} // namespace
int main() {
  try {
    Test();
  } catch (const std::exception &error) {
    std::fprintf(stderr, "%s\n", error.what());
    return 1;
  }
}
