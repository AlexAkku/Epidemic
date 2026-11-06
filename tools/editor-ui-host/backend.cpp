#include "demo_game.h"
#include "Epidemic/Game/Editor/editor_workspace.h"
#include "Epidemic/Game/Editor/editor_inspector.h"
#include "Epidemic/Game/Editor/content_browser.h"
#include "Epidemic/Game/Editor/content_dependencies.h"
#include "Epidemic/Game/Editor/content_thumbnail_cache.h"
#include "Epidemic/Game/Editor/project_path.h"
#include "Epidemic/Game/Editor/scene_view.h"
#include <bit>
#include <cmath>
#include <array>
#include <charconv>
#include <chrono>
#include <cstdio>
#include <fcntl.h>
#include <io.h>
#include <iostream>
#include <limits>
#include <span>
#include <set>
#include <unordered_map>
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
    std::uint64_t component = 0, part = 0;
};

class Backend {
    ex::Schema schema_;
    std::unique_ptr<ed::EditorWorkspace> workspace_;
    std::uint64_t generation_ =
        Id::FromString(std::to_string(GetCurrentProcessId()) + "." +
                       std::to_string(std::chrono::system_clock::now().time_since_epoch().count()))
            .Raw() >>
        1u;
    std::uint64_t selection_revision_ = 0;
    std::uint64_t document_nonce_ = 0;
    mutable ed::ThumbnailCache thumbnails_;
    std::string asset_query_;

    static void CheckDocumentName(std::string_view name) {
        if (name.empty() || name.size() > 256 ||
            name.find_first_not_of(" \t\r\n") == std::string_view::npos ||
            name.find_first_of("\r\n\t") != std::string_view::npos)
            throw std::runtime_error("ui.document_name");
    }

    void CreateDocument(const std::vector<std::string> &args, std::uint64_t revision) {
        const auto kind = Numeric(args.at(0));
        if (kind > 2)
            throw std::runtime_error("ui.document_kind");
        CheckDocumentName(args.at(1));
        const auto nonce = std::to_string(GetCurrentProcessId()) + "." +
                           std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()) +
                           "." + std::to_string(++document_nonce_);
        const auto id = rt::AssetId::FromString("editor.document." + nonce);
        ed::EditorDocument document;
        document.id = id;
        document.name = args.at(1);
        if (kind == 0) {
            epidemic::game::composition::SceneCompositionDefinition scene;
            scene.asset_id = id;
            document.data = std::move(scene);
        } else if (kind == 1) {
            // Use the current registered schema, without demo components/behaviors.
            // User-defined archetype authoring belongs to F4.
            ed::comp::PrefabDefinition prefab;
            prefab.id = id.value;
            prefab.root.id = Id::FromString("editor.root." + nonce);
            prefab.root.archetype = schema_.enemy;
            if (args.size() == 3) {
                if (args[2] != "registered-components")
                    throw std::runtime_error("ui.prefab_template");
                // Real registered component recipes, no behavior/graph fixture.
                prefab.root.components = ex::Enemy(schema_, prefab.root.id).components;
            }
            document.data = std::move(prefab);
        } else {
            rt::graph::GraphDefinition graph;
            graph.asset_id = id;
            graph.name = document.name;
            document.data = std::move(graph);
        }
        Require(workspace_->Execute({ed::CreateDocument{std::move(document)}}, revision));
        if (!workspace_->Select({id, ed::UiElement::Document, 0}))
            throw std::runtime_error("ui.selection_missing");
        ++selection_revision_;
    }

    std::vector<ed::InspectorEntry> Inspector() const {
        if (!workspace_->Project())
            return {};
        auto rows = ed::InspectComponents(*workspace_->Project()->Snapshot(),
                                          workspace_->Selection(), schema_.components, nullptr);
        for (auto &row : rows)
            if (!row.value) {
                row.editable = false;
                row.readonly_reason = "Factory default unavailable";
            }
        if (workspace_->Session() && workspace_->Session()->IsPlaying())
            for (auto &row : rows) {
                row.editable = false;
                row.readonly_reason = "Stop Play to edit authoring";
            }
        return rows;
    }

    static std::string FieldKey(const ed::InspectorEntry &row, std::size_t ordinal = 0) {
        return std::to_string(row.owner.document.Raw()) + "/" + std::to_string(row.owner.element) +
               "/" + std::to_string(row.owner.component.Raw()) + "/" +
               std::to_string(row.owner.part.Raw()) + "/" + std::to_string(row.property.Raw()) +
               "/" + std::to_string(static_cast<unsigned>(row.target)) + "/" +
               std::to_string(row.placement.document.Raw()) + "/" +
               std::to_string(row.placement.element) +
               (row.kind ? "" : "/readonly/" + std::to_string(ordinal));
    }

    void Reset() {
        thumbnails_.Clear();
        asset_query_.clear();
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
                      (workspace_->Session() && workspace_->Session()->IsPlaying() ? 8u : 0u) |
                      (workspace_->AssetJobsBusy() ? 16u : 0u) |
                      (workspace_->ContentWatchEnabled() ? 32u : 0u));
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
        out.Number<8>(selection.component.Raw());
        out.Number<8>(selection.part.Raw());
        out.Number<4>(snapshot->definition.documents.size());
        std::vector<Node> nodes;
        for (const auto &document : snapshot->definition.documents) {
            out.Number<8>(document.id.Raw());
            out.Number<8>(document.revision);
            out.Number<1>(static_cast<unsigned>(document.Kind()));
            out.Text(document.name);
            out.Number<1>(project->IsDirty(document.id) ? 1 : 0);
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
                    for (const auto &component : node->components) {
                        const auto *info = schema_.components.Find(component.type);
                        nodes.push_back({document.id.Raw(), node->id.Raw(), node->id.Raw(),
                                         ed::UiElement::PrefabComponent,
                                         info ? info->name : "Component", component.type.Raw(),
                                         component.part.Raw()});
                    }
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
            out.Number<8>(node.component);
            out.Number<8>(node.part);
        }
        const auto *folder = workspace_->Folder();
        std::vector<const ed::ProjectSourceAsset *> visible;
        if (folder) {
            const auto listing =
                ed::BrowseContent(*folder,
                                  {.search = asset_query_, .show_trashed = true, .limit = 1000});
            std::unordered_map<std::uint64_t, const ed::ProjectSourceAsset *> indexed;
            for (const auto &source : folder->Sources())
                indexed.emplace(source.id.Raw(), &source);
            std::set<std::uint64_t> matching;
            for (const auto &item : listing.items)
                matching.insert(item.id.Raw());
            for (const auto &source : folder->Sources())
                if (matching.contains(source.id.Raw()))
                    visible.push_back(&source);
            if (selection.kind == ed::UiElement::Asset && indexed.contains(selection.element) &&
                std::none_of(visible.begin(), visible.end(), [&](const auto *asset) {
                    return asset->id.Raw() == selection.element;
                }))
                visible.push_back(indexed.at(selection.element));
        }
        out.Number<4>(visible.size());
        for (const auto *asset : visible) {
            const auto &source = *asset;
            out.Number<8>(source.id.Raw());
            out.Number<8>(source.type.value.Raw());
            out.Number<8>(source.generation);
            out.Number<8>(source.source_hash);
            out.Number<1>(source.IsTrashed() ? 1 : 0);
            out.Text(source.relative_path);
            out.Text(source.cooked_key);
            out.Text(source.original_path);
            out.Text(selection.kind == ed::UiElement::Asset &&
                             selection.element == source.id.Raw() && !source.cooked_key.empty()
                         ? ed::CookedMediaMetadata(folder->Root(), source.cooked_key)
                         : "");
            const auto resolved = ed::ResolveProjectPath(folder->Root(), source.relative_path);
            std::error_code fs_error;
            out.Number<1>(resolved &&
                          std::filesystem::is_regular_file(resolved.Value(), fs_error) &&
                          !fs_error);
        }
        const auto fields = Inspector();
        out.Number<4>(fields.size());
        for (std::size_t index = 0; index < fields.size(); ++index) {
            const auto &row = fields[index];
            out.Text(FieldKey(row, index));
            out.Text(row.name);
            out.Number<1>(row.kind ? static_cast<unsigned>(*row.kind) : 255);
            out.Text(row.value ? ed::EditorWorkspace::FormatValue(*row.value)
                               : "<value unavailable>");
            out.Number<1>(row.editable ? 1 : 0);
            out.Text(row.source);
            out.Text(row.readonly_reason);
            out.Number<8>(row.reference.Raw());
        }

        struct DrawSprite {
            std::uint64_t document = 0;
            std::string key;
            rt::Transform world;
            ed::PrefabSpriteBinding binding;
        };

        std::vector<DrawSprite> sprites;
        if (folder) {
            const auto append = [&](const ed::EditorDocument &doc, rt::AssetId prefab,
                                    const rt::Transform &world) {
                for (const auto &binding : folder->SpriteBindings())
                    if (binding.prefab == prefab) {
                        const auto found =
                            std::find_if(folder->Sources().begin(), folder->Sources().end(),
                                         [&](const auto &asset) {
                                             return asset.id == binding.texture &&
                                                    !asset.IsTrashed() &&
                                                    ed::IsImportedTexture(asset.type);
                                         });
                        if (found != folder->Sources().end() && !found->cooked_key.empty())
                            sprites.push_back({doc.id.Raw(), found->cooked_key, world, binding});
                    }
            };
            for (const auto &doc : snapshot->definition.documents) {
                if (doc.Kind() == ed::DocumentKind::Prefab)
                    append(doc, doc.id, {});
                else if (const auto *scene =
                             std::get_if<epidemic::game::composition::SceneCompositionDefinition>(
                                 &doc.data)) {
                    ed::SceneView projection;
                    Require(projection.Update(*snapshot, doc.id));
                    for (const auto &placement : scene->placements)
                        for (const auto &item : projection.Items())
                            if (item.id == placement.node)
                                append(doc, placement.prefab, item.world);
                }
            }
        }
        out.Number<4>(sprites.size());
        for (const auto &sprite : sprites) {
            out.Number<8>(sprite.document);
            out.Text(sprite.key);
            const double values[]{sprite.world.position.x,
                                  sprite.world.position.y,
                                  sprite.world.scale.x,
                                  sprite.world.scale.y,
                                  2.0 *
                                      std::atan2(sprite.world.rotation.z, sprite.world.rotation.w) *
                                      180.0 / 3.141592653589793,
                                  sprite.binding.pixels_per_unit,
                                  sprite.binding.pivot_x,
                                  sprite.binding.pivot_y};
            for (const auto value : values)
                out.Number<8>(std::bit_cast<std::uint64_t>(value));
        }
        out.Number<4>(folder ? folder->Sources().size() : 0);
        out.Number<8>(workspace_->ContentRevision());
        const auto events = workspace_->ContentEventsSince(0);
        out.Number<4>(events.size());
        for (const auto &event : events) {
            out.Number<8>(event.id.Raw());
            out.Number<1>(static_cast<unsigned>(event.kind));
            out.Text(event.diagnostic);
            out.Number<8>(event.job);
            out.Text(event.source);
        }
        const auto references = folder ? ed::BuildContentDependencyIndex(*folder).references
                                       : std::vector<ed::ContentReference>{};
        out.Number<4>(references.size());
        for (const auto &reference : references) {
            out.Number<8>(reference.source.Raw());
            out.Number<8>(reference.dependent.Raw());
            out.Text(reference.origin);
            out.Number<1>(reference.required);
        }
        ed::ThumbnailPixels preview;
        std::string preview_error;
        if (folder && selection.kind == ed::UiElement::Asset) {
            for (const auto &asset : folder->Sources())
                if (asset.id.Raw() == selection.element && !asset.cooked_key.empty() &&
                    ed::IsImportedTexture(asset.type)) {
                    const auto cached = thumbnails_.Get(asset.cooked_key);
                    if (cached)
                        preview = *cached;
                    else {
                        ed::ImageResourceLoader loader(folder->Root());
                        const auto id = loader.Register(asset.cooked_key);
                        if (id) {
                            const auto loaded = loader.Load({id.Value(), loader.GetResourceType()});
                            if (loaded) {
                                const auto payload =
                                    std::dynamic_pointer_cast<const ed::ImagePayload>(
                                        loaded.Value().payload);
                                if (payload) {
                                    const auto thumb = ed::MakeImageThumbnail(payload->image);
                                    preview = {thumb.width, thumb.height, thumb.pixels};
                                    (void)thumbnails_.Put(asset.cooked_key, preview);
                                }
                            } else
                                preview_error = loaded.GetError().code;
                        } else
                            preview_error = id.GetError().code;
                    }
                }
        }
        out.Number<4>(preview.width);
        out.Number<4>(preview.height);
        std::string pixels;
        pixels.reserve(preview.bgra.size() * 2);
        constexpr char hex[] = "0123456789abcdef";
        for (const auto byte : preview.bgra) {
            const auto value = std::to_integer<unsigned>(byte);
            pixels.push_back(hex[value >> 4u]);
            pixels.push_back(hex[value & 15u]);
        }
        out.Text(pixels);
        out.Text(preview_error);
    }

  public:
    Backend() {
        Reset();
    }

    bool Request(const Bytes &bytes) {
        std::uint64_t correlation = 0;
        std::string error;
        bool quit = false;
        const auto previous_selection = workspace_->Selection();
        const auto previous_selection_revision = selection_revision_;
        try {
            Reader in{bytes};
            if (in.Number(4) != Magic || in.Number(2) != 3 || in.Number(2) != 0)
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
            if ((operation == 5 && args.size() != 2) ||
                (operation == 15 && args.size() != 2 && args.size() != 3) ||
                (operation == 16 && args.size() != 3) || (operation == 17 && args.size() != 1) ||
                (operation == 9 && args.size() != 3 && args.size() != 5) ||
                (operation == 12 && args.size() != 1) ||
                ((operation == 13 || operation == 14) && !args.empty()))
                throw std::runtime_error("ui.argument_count");
            if (operation >= 18 && operation <= 24) {
                if (args.empty() || Numeric(args[0]) != workspace_->AssetRevision())
                    throw std::runtime_error("ui.stale_asset_revision");
                CheckWrite({generation, revision});
            }
            if ((operation >= 5 && operation <= 9) || (operation >= 12 && operation <= 17))
                CheckWrite({generation, revision});
            if ((operation == 5 || (operation >= 15 && operation <= 24)) && workspace_->Session() &&
                workspace_->Session()->IsPlaying())
                throw std::runtime_error("ui.playing_readonly");
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
                CheckDocumentName(args.at(1));
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
                if (kind > static_cast<unsigned>(ed::UiElement::Asset))
                    throw std::runtime_error("ui.selection_kind");
                if (!workspace_->Select(
                        {Asset(Numeric(args.at(1))), static_cast<ed::UiElement>(kind),
                         Numeric(args.at(2)),
                         args.size() == 5 ? epidemic::gameplay::TypeId{Numeric(args[3])}
                                          : epidemic::gameplay::TypeId{},
                         args.size() == 5
                             ? epidemic::gameplay::entities::EntityPartId{Numeric(args[4])}
                             : epidemic::gameplay::entities::EntityPartId{}}))
                    throw std::runtime_error("ui.selection_missing");
                ++selection_revision_;
                break;
            }
            case 18:
                if (args.size() != 3)
                    throw std::runtime_error("ui.argument_count");
                Require(workspace_->QueueImportPng(Path(args[1]), args[2]));
                break;
            case 19:
                if (args.size() != 2)
                    throw std::runtime_error("ui.argument_count");
                Require(workspace_->ReimportPng(Asset(Numeric(args[1]))));
                break;
            case 20:
                if (args.size() != 3)
                    throw std::runtime_error("ui.argument_count");
                Require(workspace_->MoveAsset(Asset(Numeric(args[1])), args[2]));
                break;
            case 21:
                if (args.size() != 3)
                    throw std::runtime_error("ui.argument_count");
                Require(workspace_->QueueDuplicate(Asset(Numeric(args[1])), args[2]));
                break;
            case 22:
                if (args.size() != 3 || (args[2] != "0" && args[2] != "1"))
                    throw std::runtime_error("ui.argument_count");
                Require(workspace_->DeleteAsset(Asset(Numeric(args[1])), args[2] == "1"));
                break;
            case 23:
                if (args.size() != 2)
                    throw std::runtime_error("ui.argument_count");
                Require(workspace_->RestoreAsset(Asset(Numeric(args[1]))));
                break;
            case 24:
                if (args.size() != 3)
                    throw std::runtime_error("ui.argument_count");
                Require(workspace_->AssignPrefabSprite(Asset(Numeric(args[1])),
                                                       Asset(Numeric(args[2]))));
                break;
            case 25:
                if (!args.empty())
                    throw std::runtime_error("ui.argument_count");
                workspace_->CancelAssetJobs();
                break;
            case 26:
                if (!args.empty())
                    throw std::runtime_error("ui.argument_count");
                (void)workspace_->PollContent();
                break;
            case 28:
                if (args.size() != 1 || args[0].size() > 1024)
                    throw std::runtime_error("ui.asset_query");
                asset_query_ = args[0];
                break;
            case 27:
                if (args.size() != 1 || (args[0] != "0" && args[0] != "1"))
                    throw std::runtime_error("ui.argument_count");
                workspace_->EnableContentWatch(args[0] == "1");
                break;
            case 12:
                Require(workspace_->CloneFolder(Path(args.at(0))));
                break;
            case 13:
                Require(workspace_->Play());
                break;
            case 14:
                Require(workspace_->Stop());
                break;
            case 15:
                if (args.size() == 3 && args[0] != "1")
                    throw std::runtime_error("ui.prefab_template");
                CreateDocument(args, revision);
                break;
            case 16: {
                if (Numeric(args[0]) != selection_revision_)
                    throw std::runtime_error("ui.stale_selection");
                const auto rows = Inspector();
                const ed::InspectorEntry *row = nullptr;
                for (std::size_t index = 0; index < rows.size(); ++index)
                    if (FieldKey(rows[index], index) == args[1]) {
                        row = &rows[index];
                        break;
                    }
                if (!row)
                    throw std::runtime_error("ui.field_missing");
                const auto command = ed::InspectorCommand(*row, args[2]);
                Require(command);
                Require(workspace_->Execute({command.Value()}, revision));
                break;
            }
            case 17:
                Require(
                    workspace_->Execute({ed::DiscardDocument{Asset(Numeric(args[0]))}}, revision));
                ++selection_revision_;
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
        if (workspace_->Selection() != previous_selection &&
            selection_revision_ == previous_selection_revision)
            ++selection_revision_;
        Wire out;
        out.Number<4>(Magic);
        out.Number<2>(3);
        out.Number<2>(0);
        out.Number<8>(correlation);
        out.Number<1>(error.empty() ? 0 : 1);
        out.Number<8>(generation_);
        out.Text("snapshot,open,new-folder,rename-document,save,undo,redo,selection,close,clone-"
                 "folder,play,stop,create-document,scalar-inspector,discard-document,prefab-"
                 "default-components,asset-pipeline");
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
