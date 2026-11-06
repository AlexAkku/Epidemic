#include "demo_game.h"
#include "Epidemic/Game/Editor/editor_workspace.h"
#include <array>
#include <charconv>
#include <chrono>
#include <cstdio>
#include <fcntl.h>
#include <io.h>
#include <iostream>
#include <limits>
#include <span>
#include <Windows.h>

namespace {
namespace ed = epidemic::game::editor;
namespace ex = ed::example;
namespace rt = epidemic::runtime;
using Id = epidemic::foundation::StringId;
constexpr std::uint32_t Magic = 0x45505549;
constexpr std::size_t MaxFrame = std::size_t{16} * 1024 * 1024;
constexpr std::size_t MaxString = std::size_t{1024} * 1024;
using Bytes = std::vector<std::uint8_t>;

struct Wire {
    Bytes data;

    template <unsigned Width> void Number(std::uint64_t value) {
        if (data.size() + Width > MaxFrame)
            throw std::runtime_error("ui.frame_limit");
        for (unsigned i = Width; i > 0; --i)
            data.push_back(static_cast<std::uint8_t>(value >> ((i - 1) * 8)));
    }

    void Text(std::string_view value) {
        if (value.size() > MaxString || data.size() + 4 + value.size() > MaxFrame)
            throw std::runtime_error("ui.string_limit");
        Number<4>(value.size());
        data.insert(data.end(), value.begin(), value.end());
    }
};

struct Reader {
    std::span<const std::uint8_t> data;
    std::size_t position = 0;

    std::uint64_t Number(unsigned width) {
        if (width > data.size() - position)
            throw std::runtime_error("ui.truncated_message");
        std::uint64_t result = 0;
        for (unsigned i = 0; i < width; ++i)
            result = (result << 8u) | static_cast<std::uint64_t>(data[position++]);
        return result;
    }

    std::string Text() {
        const auto size = Number(4);
        if (size > MaxString || size > data.size() - position)
            throw std::runtime_error("ui.string_limit");
        std::string result(reinterpret_cast<const char *>(data.data() + position),
                           static_cast<std::size_t>(size));
        if (result.find('\0') != std::string::npos ||
            (!result.empty() && !MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, result.data(),
                                                     static_cast<int>(result.size()), nullptr, 0)))
            throw std::runtime_error("ui.invalid_utf8_text");
        position += static_cast<std::size_t>(size);
        return result;
    }
};

std::filesystem::path Path(std::string_view value) {
    return std::filesystem::path{
        std::u8string(reinterpret_cast<const char8_t *>(value.data()), value.size())};
}

std::string Utf8(const std::filesystem::path &value) {
    const auto text = value.u8string();
    return {reinterpret_cast<const char *>(text.data()), text.size()};
}

std::uint64_t Numeric(std::string_view value) {
    std::uint64_t result = 0;
    const auto parsed = std::from_chars(value.data(), value.data() + value.size(), result);
    if (parsed.ec != std::errc{} || parsed.ptr != value.data() + value.size())
        throw std::runtime_error("ui.invalid_id");
    return result;
}

rt::AssetId Asset(std::uint64_t value) {
    return rt::AssetId{Id{value}};
}

template <class T> void Require(const T &value) {
    if (!value)
        throw std::runtime_error(value.GetError().code);
}

struct Node {
    std::uint64_t document = 0, element = 0, parent = 0;
    ed::UiElement kind = ed::UiElement::Document;
    std::string label;
};

class Backend {
    ex::Schema schema_;
    std::unique_ptr<ed::EditorWorkspace> workspace_;
    std::uint64_t generation_ = 1, selection_revision_ = 0;

    void Reset() {
        workspace_.reset();
        workspace_ = std::make_unique<ed::EditorWorkspace>(schema_.View(),
                                                           [this] { return ex::Options(schema_); });
        ++generation_;
        ++selection_revision_;
    }

    struct Version {
        std::uint64_t generation, revision;
    };

    void CheckWrite(Version expected) const {
        if (!workspace_->Project())
            throw std::runtime_error("ui.no_project");
        if (expected.generation != generation_)
            throw std::runtime_error("ui.stale_generation");
        if (!expected.revision || expected.revision != workspace_->Project()->Revision())
            throw std::runtime_error("ui.stale_revision");
    }

    void NewFolder(const std::vector<std::string> &args) {
        if (workspace_->Project())
            throw std::runtime_error("ui.close_project_first");
        auto destination = std::filesystem::absolute(Path(args.at(0))).lexically_normal();
        const auto parent = destination.parent_path();
        if (!std::filesystem::is_directory(parent))
            throw std::runtime_error("ui.parent_missing");
        if (std::filesystem::exists(destination))
            throw std::runtime_error("editor.folder_exists");
        const auto nonce =
            std::to_string(std::chrono::steady_clock::now().time_since_epoch().count());
        const auto envelope = parent / (".epidemic-create-" + nonce);
        // Exclusive ownership of this container makes cleanup independent of a
        // competing final destination. Never remove the requested destination.
        if (!std::filesystem::create_directory(envelope))
            throw std::runtime_error("ui.staging_collision");
        try {
            const auto scene_id = rt::AssetId::FromString("ui.scene." + nonce);
            epidemic::game::composition::SceneCompositionDefinition scene;
            scene.asset_id = scene_id;
            ed::ProjectDefinition definition{Id::FromString("ui.project." + nonce),
                                             scene_id,
                                             {{scene_id, "Scene", 0, scene}}};
            Require(workspace_->NewFolder(envelope / "project", args.at(1), std::move(definition)));
            Reset(); // Release the staging writer lock before publication.
            if (std::filesystem::exists(destination))
                throw std::runtime_error("editor.folder_exists");
            if (!MoveFileW((envelope / "project").c_str(), destination.c_str()))
                throw std::system_error(static_cast<int>(GetLastError()), std::system_category(),
                                        "ui.project_publish"); // Never replace a competing
                                                               // destination.
            Require(workspace_->OpenFolder(destination));
            std::error_code ignored;
            std::filesystem::remove(envelope, ignored);
        } catch (...) {
            Reset();
            std::error_code ignored;
            std::filesystem::remove_all(envelope, ignored);
            throw;
        }
    }

    void Snapshot(Wire &out) const {
        const auto *project = workspace_->Project();
        out.Number<1>(project ? 1 : 0);
        if (!project)
            return;
        const auto snapshot = project->Snapshot();
        out.Number<8>(project->Revision());
        out.Number<8>(workspace_->AssetRevision());
        out.Number<8>(selection_revision_);
        out.Number<1>((workspace_->IsDirty() ? 1u : 0u) | (workspace_->CanUndo() ? 2u : 0u) |
                      (workspace_->CanRedo() ? 4u : 0u) |
                      (workspace_->Session() && workspace_->Session()->IsPlaying() ? 8u : 0u));
        out.Number<1>(workspace_->Folder() ? 2 : 1);
        out.Number<8>(snapshot->definition.id.Raw());
        out.Number<8>(snapshot->definition.entry_scene.Raw());
        out.Text(Utf8(workspace_->Folder() ? workspace_->Folder()->Root() / "Project.epidemicproj"
                                           : project->File()));
        out.Text(workspace_->Folder() ? workspace_->Folder()->Title()
                                      : Utf8(project->File().stem()));
        const auto selection = workspace_->Selection();
        out.Number<1>(static_cast<unsigned>(selection.kind));
        out.Number<8>(selection.document.Raw());
        out.Number<8>(selection.element);
        out.Number<4>(snapshot->definition.documents.size());
        std::vector<Node> nodes;
        for (const auto &document : snapshot->definition.documents) {
            out.Number<8>(document.id.Raw());
            out.Number<8>(document.revision);
            out.Number<1>(static_cast<unsigned>(document.Kind()));
            out.Text(document.name);
            if (const auto *scene =
                    std::get_if<epidemic::game::composition::SceneCompositionDefinition>(
                        &document.data)) {
                for (const auto &node : scene->scene.nodes)
                    nodes.push_back({document.id.Raw(), node.id.Raw(), node.parent.Raw(),
                                     ed::UiElement::SceneNode,
                                     "Entity " + std::to_string(node.id.Raw())});
            } else if (const auto *prefab =
                           std::get_if<ed::comp::PrefabDefinition>(&document.data)) {
                std::vector<std::pair<const ed::comp::PrefabNode *, std::uint64_t>> pending{
                    {&prefab->root, 0}};
                while (!pending.empty()) {
                    const auto [node, parent] = pending.back();
                    pending.pop_back();
                    if (!node->id.Raw())
                        continue;
                    nodes.push_back({document.id.Raw(), node->id.Raw(), parent,
                                     ed::UiElement::PrefabNode,
                                     "Prefab entity " + std::to_string(node->id.Raw())});
                    for (const auto &child : node->children)
                        pending.emplace_back(&child, node->id.Raw());
                }
            } else if (const auto *graph =
                           std::get_if<rt::graph::GraphDefinition>(&document.data)) {
                for (const auto &node : graph->nodes)
                    nodes.push_back({document.id.Raw(), node.id.Raw(), 0, ed::UiElement::GraphNode,
                                     "Graph node " + std::to_string(node.id.Raw())});
            }
        }
        out.Number<4>(nodes.size());
        for (const auto &node : nodes) {
            out.Number<8>(node.document);
            out.Number<8>(node.element);
            out.Number<8>(node.parent);
            out.Number<1>(static_cast<unsigned>(node.kind));
            out.Text(node.label);
        }
        const auto *folder = workspace_->Folder();
        out.Number<4>(folder ? folder->Sources().size() : 0);
        if (folder)
            for (const auto &source : folder->Sources()) {
                out.Number<8>(source.id.Raw());
                out.Number<8>(source.type.value.Raw());
                out.Number<8>(source.generation);
                out.Number<8>(source.source_hash);
                out.Number<1>(source.IsTrashed() ? 1 : 0);
                out.Text(source.relative_path);
                out.Text(source.cooked_key);
            }
    }

  public:
    Backend() {
        Reset();
    }

    bool Request(const Bytes &bytes) {
        std::uint64_t correlation = 0;
        std::string error;
        bool quit = false;
        try {
            Reader in{bytes};
            if (in.Number(4) != Magic || in.Number(2) != 1 || in.Number(2) != 0)
                throw std::runtime_error("ui.protocol_version");
            correlation = in.Number(8);
            const auto operation = in.Number(1), generation = in.Number(8), revision = in.Number(8);
            const auto count = in.Number(2);
            if (count > 8)
                throw std::runtime_error("ui.argument_limit");
            std::vector<std::string> args;
            for (std::uint64_t i = 0; i < count; ++i)
                args.push_back(in.Text());
            if (in.position != bytes.size())
                throw std::runtime_error("ui.trailing_bytes");
            if (operation != 1 && operation != 4 && generation != generation_)
                throw std::runtime_error("ui.stale_generation");
            if ((operation == 12 && args.size() != 1) ||
                ((operation == 13 || operation == 14) && !args.empty()))
                throw std::runtime_error("ui.argument_count");
            if ((operation >= 5 && operation <= 9) || (operation >= 12 && operation <= 14))
                CheckWrite({generation, revision});
            switch (operation) {
            case 1:
            case 4:
                break;
            case 2:
                if (workspace_->Project())
                    throw std::runtime_error("ui.close_project_first");
                Require(workspace_->Open(Path(args.at(0))));
                ++generation_;
                ++selection_revision_;
                break;
            case 3:
                NewFolder(args);
                break;
            case 5:
                Require(workspace_->Execute({ed::RenameDocument{Asset(Numeric(args.at(0))),
                                                                args.at(1)}},
                                            revision));
                break;
            case 6:
                if (workspace_->IsDirty())
                    Require(workspace_->Save());
                break;
            case 7:
                Require(workspace_->Undo());
                break;
            case 8:
                Require(workspace_->Redo());
                break;
            case 9: {
                const auto kind = Numeric(args.at(0));
                if (kind > static_cast<unsigned>(ed::UiElement::Connection))
                    throw std::runtime_error("ui.selection_kind");
                if (!workspace_->Select({Asset(Numeric(args.at(1))),
                                         static_cast<ed::UiElement>(kind), Numeric(args.at(2))}))
                    throw std::runtime_error("ui.selection_missing");
                ++selection_revision_;
                break;
            }
            case 12:
                Require(workspace_->CloneFolder(Path(args.at(0))));
                break;
            case 13:
                Require(workspace_->Play());
                break;
            case 14:
                Require(workspace_->Stop());
                break;
            case 10:
            case 11:
                if (workspace_->IsDirty() &&
                    !(operation == 10 && args.size() == 1 && args[0] == "discard"))
                    throw std::runtime_error("ui.unsaved_project");
                Reset();
                quit = operation == 11;
                break;
            default:
                throw std::runtime_error("ui.unavailable_operation");
            }
        } catch (const std::exception &failure) {
            error = failure.what();
        }
        Wire out;
        out.Number<4>(Magic);
        out.Number<2>(1);
        out.Number<2>(0);
        out.Number<8>(correlation);
        out.Number<1>(error.empty() ? 0 : 1);
        out.Number<8>(generation_);
        out.Text("snapshot,open,new-folder,rename-document,save,undo,redo,selection,close,clone-folder,play,stop");
        out.Text(error);
        Snapshot(out);
        Wire prefix;
        prefix.Number<4>(out.data.size());
        std::cout.write(reinterpret_cast<const char *>(prefix.data.data()), 4);
        std::cout.write(reinterpret_cast<const char *>(out.data.data()),
                        static_cast<std::streamsize>(out.data.size()));
        std::cout.flush();
        return !quit && static_cast<bool>(std::cout);
    }
};
} // namespace

int main() {
    _setmode(_fileno(stdin), _O_BINARY);
    _setmode(_fileno(stdout), _O_BINARY);
    try {
        Backend backend;
        for (;;) {
            std::array<std::uint8_t, 4> prefix{};
            std::cin.read(reinterpret_cast<char *>(prefix.data()), 4);
            if (!std::cin)
                return std::cin.gcount() == 0 && std::cin.eof() ? 0 : 1;
            Reader length{prefix};
            const auto size = length.Number(4);
            if (size == 0 || size > MaxFrame)
                return 2;
            Bytes body(static_cast<std::size_t>(size));
            if (!std::cin.read(reinterpret_cast<char *>(body.data()),
                               static_cast<std::streamsize>(size)))
                return 2;
            if (!backend.Request(body))
                return 0;
        }
    } catch (const std::exception &error) {
        std::fprintf(stderr, "Editor bridge: %s\n", error.what());
        return 1;
    }
}
