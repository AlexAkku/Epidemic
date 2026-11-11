#include "demo_game.h"
#include "Epidemic/Game/Editor/editor_workspace.h"
#include "Epidemic/Game/Editor/editor_inspector.h"
#include "Epidemic/Game/Editor/content_browser.h"
#include "Epidemic/Game/Editor/content_dependencies.h"
#include "Epidemic/Game/Editor/content_thumbnail_cache.h"
#include "Epidemic/Game/Editor/project_path.h"
#include "Epidemic/Game/Editor/scene_view.h"
#include "Epidemic/Game/Editor/scene_drag.h"
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
    std::uint64_t prefab = 0;
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
    mutable std::chrono::steady_clock::time_point refresh_content_folders_{};
    mutable std::vector<std::string> content_folders_;
    ed::SceneDrag drag_;
    ed::SceneView drag_view_;
    mutable std::weak_ptr<const ed::ProjectSnapshot> projected_source_;
    mutable std::map<std::uint64_t,ed::SceneView> scene_projections_;

    static double Real(std::string_view value) {
        const auto parsed = ed::EditorWorkspace::ParseValue(ed::comp::PropertyKind::Real,value);
        Require(parsed);
        return std::get<double>(parsed.Value());
    }

    rt::SceneNodeId NewSceneNode() {
        return rt::SceneNodeId{Id::FromString("editor.scene.node." + std::to_string(generation_) +
                                             "." + std::to_string(++document_nonce_)).Raw()};
    }

    void EditSystems(const std::vector<std::string> &args,std::uint64_t revision) {
        if (args.size() == 3 && args[0] == "add") {
            const auto scope = Asset(Numeric(args[1]));
            const auto id = Id::FromString("editor.system." + std::to_string(generation_) + "." + std::to_string(++document_nonce_));
            Require(workspace_->Execute({ed::EditSystem{scope,{id,scope,Asset(Numeric(args[2])),true}}},revision));
            return;
        }
        if (args.size() < 2)
            throw std::runtime_error("ui.argument_count");
        const auto snap = workspace_->Project()->Snapshot();
        const auto id = Id{Numeric(args[1])};
        const auto found = std::ranges::find(snap->definition.systems,id,&ed::ProjectSystem::id);
        if (found == snap->definition.systems.end())
            throw std::runtime_error("ui.system_missing");
        auto system = *found;
        bool remove = false;
        if (args[0] == "remove" && args.size() == 2)
            remove = true;
        else if (args[0] == "enable" && args.size() == 3 && (args[2] == "1" || args[2] == "0"))
            system.enabled = args[2] == "1";
        else if (args[0] == "graph" && args.size() == 3)
            system.graph = Asset(Numeric(args[2]));
        else
            throw std::runtime_error("ui.system_command");
        Require(workspace_->Execute({ed::EditSystem{system.scene,system,remove}},revision));
    }

    void PlaceResource(const std::vector<std::string> &args, std::uint64_t revision) {
        const auto snap = workspace_->Project()->Snapshot();
        if (args.size() == 5 && args[0] == "place") {
            const auto document = Asset(Numeric(args[1]));
            const auto prefab = Asset(Numeric(args[2]));
            const auto x = Real(args[3]), y = Real(args[4]);
            if (std::abs(x) > std::numeric_limits<float>::max() || std::abs(y) > std::numeric_limits<float>::max())
                throw std::runtime_error("ui.placement_range");
            rt::SceneNodeSnapshot node;
            node.id = NewSceneNode();
            node.local_transform.position.x = static_cast<float>(x);
            node.local_transform.position.y = static_cast<float>(y);
            Require(workspace_->Execute({ed::AddSceneNode{document,node},ed::PlacePrefab{document,{node.id,prefab,{}}}},revision));
            (void)workspace_->Select({document,ed::UiElement::SceneNode,node.id.Raw()});
        } else if (args.size() == 5 && args[0] == "sprite") {
            if (Numeric(args[1]) != workspace_->AssetRevision())
                throw std::runtime_error("ui.stale_asset_revision");
            const auto document = Asset(Numeric(args[2]));
            const auto node = rt::SceneNodeId{Numeric(args[3])};
            const auto *doc = snap->Find(document);
            if (!doc)
                throw std::runtime_error("ui.sprite_owner");
            auto prefab = document;
            if (const auto *scene = std::get_if<epidemic::game::composition::SceneCompositionDefinition>(&doc->data)) {
                const auto placement = std::ranges::find(scene->placements,node,&epidemic::game::composition::ScenePrefabPlacement::node);
                if (placement == scene->placements.end())
                    throw std::runtime_error("ui.sprite_requires_prefab");
                prefab = placement->prefab;
            }
            Require(workspace_->AssignPrefabSprite(prefab,Asset(Numeric(args[4]))));
        } else {
            throw std::runtime_error("ui.resource_command");
        }
    }

    void SceneGesture(const std::vector<std::string> &args) {
        if (args.empty())
            throw std::runtime_error("ui.argument_count");
        if (args[0] == "cancel" && args.size() == 1) {
            drag_.Cancel();
        } else if (args[0] == "begin" && args.size() == 8) {
            drag_.Cancel();
            const auto selected = workspace_->Selection();
            const auto doc = Asset(Numeric(args[1]));
            if (selected.document != doc || selected.kind != ed::UiElement::SceneNode)
                throw std::runtime_error("ui.scene_selection");
            const std::string_view mode_token = args[2];
            const auto separator = mode_token.find(':');
            if (separator == std::string_view::npos || Numeric(mode_token.substr(separator+1)) != selection_revision_)
                throw std::runtime_error("ui.stale_selection");
            const auto tool = Numeric(mode_token.substr(0,separator));
            if (tool > 2)
                throw std::runtime_error("ui.drag_tool");
            const auto snap = workspace_->Project()->Snapshot();
            Require(drag_view_.Update(*snap, doc));
            std::string_view camera = args[7];
            const auto first = camera.find(',');
            const auto last = camera.rfind(',');
            if (first == std::string_view::npos || first == last ||
                !drag_view_.RestoreCamera({Real(args[5]),Real(args[6])},
                    {Real(camera.substr(0,first)),Real(camera.substr(first+1,last-first-1))},Real(camera.substr(last+1))))
                throw std::runtime_error("ui.drag_camera");
            const auto item = std::ranges::find(drag_view_.Items(), rt::SceneNodeId{selected.element}, &ed::SceneViewItem::id);
            if (item == drag_view_.Items().end())
                throw std::runtime_error("ui.scene_selection");
            const ed::ViewPoint point{Real(args[3]),Real(args[4])};
            const auto mode = static_cast<ed::GizmoTool>(tool);
            const auto space = ed::GizmoSpace::Local;
            const auto handle = ed::PickGizmo(ed::MakeGizmo(drag_view_, item->world, space),mode,point);
            Require(drag_.Begin(*workspace_,drag_view_,mode,space,handle,point));
        } else if ((args[0] == "move" || args[0] == "commit") && args.size() == 4) {
            if (!drag_.Current(*workspace_))
                throw std::runtime_error("ui.stale_gesture");
            const auto result = drag_.Move(drag_view_,{Real(args[1]),Real(args[2])},Real(args[3]));
            if (args[0] == "commit") {
                if (!result)
                    drag_.Cancel();
                Require(result);
                Require(drag_.Commit(*workspace_));
            } else
                Require(result);
        } else {
            throw std::runtime_error("ui.gesture_command");
        }
    }

    static void CheckDocumentName(std::string_view name) {
        if (name.empty() || name.size() > 256 ||
            name.find_first_not_of(" \t\r\n") == std::string_view::npos ||
            name.find_first_of("\r\n\t") != std::string_view::npos)
            throw std::runtime_error("ui.document_name");
    }

    void EditSceneHierarchy(const std::vector<std::string> &args, std::uint64_t revision) {
        if (args.size() < 3 || args.size() > 4)
            throw std::runtime_error("ui.argument_count");
        const auto document = Asset(Numeric(args[1]));
        const auto node = rt::SceneNodeId{Numeric(args[2])};
        const auto snapshot = workspace_->Project()->Snapshot();
        const auto *doc = snapshot->Find(document);
        if (args[0] == "prefab-parent" && args.size() == 4) {
            Require(workspace_->Execute({ed::ReparentPrefabNode{document,ed::comp::PrefabNodeId{node.Raw()},
                ed::comp::PrefabNodeId{Numeric(args[3])}}},revision));
            return;
        }
        if (args[0] == "link-prefab" && args.size() == 4) {
            const auto id = Id::FromString("editor.prefab.link." + std::to_string(generation_) +
                                           "." + std::to_string(++document_nonce_));
            Require(workspace_->Execute({ed::EditPrefabLink{document,
                {id,document,ed::comp::PrefabNodeId{node.Raw()},Asset(Numeric(args[3]))},false}},revision));
            (void)workspace_->Select({document,ed::UiElement::PrefabLink,id.Raw()});
            return;
        }
        if (args[0] == "unlink-prefab" && args.size() == 3) {
            Require(workspace_->Execute({ed::EditPrefabLink{document,{Id{node.Raw()},document,{},{}},true}},revision));
            return;
        }
        using Scene = epidemic::game::composition::SceneCompositionDefinition;
        const auto *scene = doc ? std::get_if<Scene>(&doc->data) : nullptr;
        if (!scene)
            throw std::runtime_error("ui.scene_missing");
        if (args[0] == "create" && args.size() == 3) {
            rt::SceneNodeSnapshot child;
            child.id = NewSceneNode();
            child.parent = node;
            Require(workspace_->Execute({ed::AddSceneNode{document, child}}, revision));
            (void)workspace_->Select({document, ed::UiElement::SceneNode, child.id.Raw()});
        } else if (args[0] == "delete" && args.size() == 3) {
            Require(workspace_->Execute({ed::DeleteSceneNode{document, node}}, revision));
        } else if (args[0] == "parent" && args.size() == 4) {
            const auto parent = rt::SceneNodeId{Numeric(args[3])};
            const auto existing = std::ranges::find(scene->scene.nodes, node, &rt::SceneNodeSnapshot::id);
            if (existing == scene->scene.nodes.end() || existing->parent != parent)
                Require(workspace_->Execute({ed::SetSceneParent{document, node, parent}}, revision));
        } else if (args[0] == "order" && args.size() == 4) {
            const std::string_view position = args[3];
            const auto separator = position.find(':');
            if (separator == std::string_view::npos)
                throw std::runtime_error("ui.scene_order");
            const auto parent = rt::SceneNodeId{Numeric(position.substr(0,separator))};
            const auto before = rt::SceneNodeId{Numeric(position.substr(separator+1))};
            if (node != before)
                Require(workspace_->Execute({ed::SetSceneParent{document,node,parent},ed::OrderSceneNode{document,node,before}},revision));
        } else if (args[0] == "rename" && args.size() == 4) {
            Require(workspace_->Execute({ed::RenameSceneNode{document, node, args[3]}}, revision));
        } else if (args[0] == "prefab" && args.size() == 4) {
            const auto prefab = Asset(Numeric(args[3]));
            for (const auto &placement : scene->placements)
                if (placement.node == node) {
                    if (placement.prefab == prefab)
                        return;
                    if (!placement.overrides.components.empty() || !placement.overrides.parameters.empty())
                        throw std::runtime_error("ui.placement_has_overrides");
                }
            Require(workspace_->Execute({ed::PlacePrefab{document, {node, prefab, {}}}}, revision));
        } else {
            throw std::runtime_error("ui.hierarchy_command");
        }
    }

    void EditComponent(const std::vector<std::string> &args, std::uint64_t revision) {
        if (args.size() < 2 || Numeric(args[0]) != selection_revision_)
            throw std::runtime_error("ui.stale_selection");
        const auto selection = workspace_->Selection();
        const auto node = ed::comp::PrefabNodeId{selection.element};
        if (args[1] == "add" && args.size() == 3) {
            ed::comp::PrefabComponent recipe;
            recipe.type = epidemic::gameplay::TypeId{Numeric(args[2])};
            auto defaults = schema_.components.Defaults(recipe.type);
            Require(defaults);
            for (auto &property : defaults.Value())
                recipe.properties.push_back({property.property, std::move(property.value), {}});
            auto commands = ed::ComponentCommands(*workspace_->Project()->Snapshot(), selection, recipe);
            Require(commands);
            Require(workspace_->Execute(std::move(commands).Value(), revision));
            (void)workspace_->Select({selection.document, ed::UiElement::PrefabComponent, selection.element, recipe.type, recipe.part});
        } else if (args[1] == "remove" && args.size() == 2 && selection.kind == ed::UiElement::PrefabComponent) {
            Require(workspace_->Execute({ed::DeleteComponent{selection.document, node, selection.component, selection.part}}, revision));
            (void)workspace_->Select({selection.document, ed::UiElement::PrefabNode, selection.element});
        } else if (args[1] == "graph" && args.size() == 3 &&
                   (selection.kind == ed::UiElement::PrefabNode || selection.kind == ed::UiElement::PrefabComponent)) {
            const auto graph = Asset(Numeric(args[2]));
            auto behavior = ed::DefaultGraphBehavior(workspace_->Project()->Snapshot()->definition,
                schema_.View(),graph,Id::FromString("editor.graph.binding." + args[2]));
            Require(behavior);
            Require(workspace_->Execute({ed::SetBehavior{selection.document, node, selection.component, selection.part, std::move(behavior).Value()}}, revision));
        } else if (args[1] == "child" && args.size() == 2 && selection.kind == ed::UiElement::PrefabNode) {
            ed::comp::PrefabNode child;
            child.id = Id::FromString("editor.prefab.node." + std::to_string(generation_) +
                                     "." + std::to_string(++document_nonce_));
            child.archetype = schema_.enemy;
            const auto id = child.id;
            Require(workspace_->Execute({ed::AddEntity{selection.document,node,std::move(child)}},revision));
            (void)workspace_->Select({selection.document,ed::UiElement::PrefabNode,id.Raw()});
        } else if (args[1] == "delete-child" && args.size() == 2 && selection.kind == ed::UiElement::PrefabNode) {
            const auto snap = workspace_->Project()->Snapshot();
            const auto *doc = snap->Find(selection.document);
            const auto *prefab = doc ? std::get_if<ed::comp::PrefabDefinition>(&doc->data) : nullptr;
            if (!prefab || prefab->root.id == node)
                throw std::runtime_error("ui.prefab_root");
            Require(workspace_->Execute({ed::DeleteEntity{selection.document,node}},revision));
        } else {
            throw std::runtime_error("ui.component_command");
        }
    }

    std::string DocumentNonce() {
        return std::to_string(GetCurrentProcessId()) + "." +
                           std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()) +
                           "." + std::to_string(++document_nonce_);
    }

    void CreateDocument(const std::vector<std::string> &args, std::uint64_t revision) {
        const auto kind = Numeric(args.at(0));
        if (kind > 2)
            throw std::runtime_error("ui.document_kind");
        CheckDocumentName(args.at(1));
        const auto nonce = DocumentNonce();
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

    void DocumentLifecycle(const std::vector<std::string> &args, std::uint64_t revision) {
        if (args.size() == 3 && args[0] == "duplicate") {
            const auto source = Asset(Numeric(args[1]));
            CheckDocumentName(args[2]);
            const auto copy = rt::AssetId::FromString("editor.document." + DocumentNonce());
            Require(workspace_->Execute({ed::DuplicateDocument{source, copy, args[2]}}, revision));
            (void)workspace_->Select({copy, ed::UiElement::Document, 0});
        } else if (args.size() == 2 && args[0] == "delete") {
            const auto id = Asset(Numeric(args[1]));
            if (workspace_->Project()->Snapshot()->definition.entry_scene == id)
                throw std::runtime_error("ui.entry_scene_delete");
            Require(workspace_->Execute({ed::DeleteDocument{id}}, revision));
        } else
            throw std::runtime_error("ui.document_command");
    }

    ed::DecodedPng AssetPreview(const std::vector<std::string> &args) const {
        if (args.size() != 3)
            throw std::runtime_error("ui.argument_count");
        const auto id = Asset(Numeric(args[0]));
        const auto divisor = Numeric(args[2]);
        if (divisor != 1 && divisor != 2 && divisor != 4)
            throw std::runtime_error("ui.preview_quality");
        const auto *folder = workspace_->Folder();
        if (!folder)
            throw std::runtime_error("ui.no_project");
        const auto &sources = folder->Sources();
        const auto asset = std::find_if(sources.begin(), sources.end(), [&](const auto &source) {
            return source.id == id && source.cooked_key == args[1] && !source.cooked_key.empty() &&
                   ed::IsImportedTexture(source.type);
        });
        if (asset == sources.end())
            throw std::runtime_error("ui.preview_stale");
        ed::ImageResourceLoader loader(folder->Root());
        const auto registered = loader.Register(asset->cooked_key);
        Require(registered);
        const auto loaded = loader.Load({registered.Value(), loader.GetResourceType()});
        Require(loaded);
        const auto payload = std::dynamic_pointer_cast<const ed::ImagePayload>(loaded.Value().payload);
        if (!payload)
            throw std::runtime_error("ui.preview_unavailable");
        const auto &image = payload->image;
        // Keep room for the normal reply header within the existing frame limit.
        if (image.pixels.size() > MaxFrame - 1024)
            throw std::runtime_error("ui.preview_bounds");
        if (divisor == 1)
            return image;
        const auto step = static_cast<std::uint32_t>(divisor);
        ed::DecodedPng reduced{(image.width + step - 1) / step, (image.height + step - 1) / step, {}};
        reduced.pixels.resize(static_cast<std::size_t>(reduced.width) * reduced.height * 4);
        for (std::uint32_t y = 0; y < reduced.height; ++y)
            for (std::uint32_t x = 0; x < reduced.width; ++x) {
                unsigned sum[4]{}, count = 0;
                for (auto sy = y * step; sy < std::min((y + 1) * step, image.height); ++sy)
                    for (auto sx = x * step; sx < std::min((x + 1) * step, image.width); ++sx) {
                        for (unsigned channel = 0; channel < 4; ++channel)
                            sum[channel] += std::to_integer<unsigned>(image.pixels[(sy * image.width + sx) * 4 + channel]);
                        ++count;
                    }
                for (unsigned channel = 0; channel < 4; ++channel)
                    reduced.pixels[(y * reduced.width + x) * 4 + channel] =
                        static_cast<std::byte>((sum[channel] + count / 2) / count);
            }
        return reduced;
    }

    std::vector<ed::InspectorEntry> Inspector() const {
        if (!workspace_->Project())
            return {};
        auto rows = ed::InspectComponents(*workspace_->Project()->Snapshot(),
                                          workspace_->Selection(), schema_.components, workspace_->Session(), true);
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

    const rt::SceneNodeSnapshot *SelectedSceneNode(const ed::ProjectSnapshot &snapshot) const {
        const auto selected = workspace_->Selection();
        const auto *doc = snapshot.Find(selected.document);
        if (selected.kind != ed::UiElement::SceneNode || !doc)
            return nullptr;
        const auto *scene = std::get_if<epidemic::game::composition::SceneCompositionDefinition>(&doc->data);
        if (!scene)
            return nullptr;
        const auto at = std::ranges::find(scene->scene.nodes, rt::SceneNodeId{selected.element}, &rt::SceneNodeSnapshot::id);
        return at == scene->scene.nodes.end() ? nullptr : &*at;
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
        drag_.Cancel();
        projected_source_.reset();
        scene_projections_.clear();
        thumbnails_.Clear();
        asset_query_.clear();
        refresh_content_folders_ = {};
        content_folders_.clear();
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

    void GraphEdit(const std::vector<std::string> &args, std::uint64_t revision) {
        namespace graph = rt::graph;
        if (args.size() < 2) throw std::runtime_error("ui.graph_arguments");
        const auto document = Asset(Numeric(args[1]));
        const auto snapshot = workspace_->Project()->Snapshot();
        const auto *doc = snapshot->Find(document);
        if (!doc || doc->Kind() != ed::DocumentKind::Graph)
            throw std::runtime_error("ui.graph_document");
        std::vector<ed::Command> commands;
        if (args[0] == "create" && args.size() == 5) {
            // Schema-owned operation templates, IDs allocated by the native owner.
            const auto template_graph = ex::Graph();
            const auto index = Numeric(args[2]);
            if (index > 2) throw std::runtime_error("ui.graph_template");
            auto node = template_graph.nodes.at(static_cast<std::size_t>(index));
            const auto nonce = DocumentNonce();
            node.id = Id::FromString("graph.node." + nonce);
            for (std::size_t i = 0; i < node.ports.size(); ++i)
                node.ports[i].id = Id::FromString("graph.port." + nonce + "." + std::to_string(i));
            commands.push_back(ed::AddGraphNode{document, node});
            commands.push_back(ed::MoveGraphNode{document, node.id, Real(args[3]), Real(args[4])});
        } else if (args[0] == "connect" && args.size() == 4) {
            commands.push_back(ed::ConnectPorts{document,
                {Id::FromString("graph.edge." + DocumentNonce()), Id{Numeric(args[2])}, Id{Numeric(args[3])}}});
        } else if (args[0] == "move" && args.size() == 5) {
            commands.push_back(ed::MoveGraphNode{document, Id{Numeric(args[2])}, Real(args[3]), Real(args[4])});
        } else if (args[0] == "delete" && args.size() == 3) {
            commands.push_back(ed::DeleteGraphNode{document, Id{Numeric(args[2])}});
        } else if (args[0] == "disconnect" && args.size() == 3) {
            commands.push_back(ed::DisconnectPorts{document, Id{Numeric(args[2])}});
        } else throw std::runtime_error("ui.graph_arguments");
        Require(workspace_->Execute(std::move(commands), revision));
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
                      (workspace_->ContentWatchEnabled() ? 32u : 0u) |
                      (workspace_->ImportsBusy() ? 64u : 0u));
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
                std::unordered_map<std::uint64_t, const ed::EditorDocument *> prefabs;
                for (const auto &placement : scene->placements)
                    if (const auto *prefab = snapshot->Find(placement.prefab))
                        prefabs.emplace(placement.node.Raw(), prefab);
                for (const auto &node : scene->scene.nodes) {
                    const auto label = scene->node_names.find(node.id.Raw());
                    auto text = label == scene->node_names.end()
                        ? "Entity " + std::to_string(node.id.Raw()) : label->second;
                    std::uint64_t prefab_id = 0;
                    if (const auto prefab = prefabs.find(node.id.Raw()); prefab != prefabs.end()) {
                        text += " · " + prefab->second->name;
                        prefab_id = prefab->second->id.Raw();
                    }
                    if (node.visibility == rt::SceneVisibilityState::Hidden)
                        text += " [hidden]";
                    nodes.push_back({document.id.Raw(), node.id.Raw(), node.parent.Raw(),
                                     ed::UiElement::SceneNode, std::move(text), 0, 0, prefab_id});
                }
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
                for (const auto &link : snapshot->definition.prefab_links)
                    if (link.owner == document.id)
                        if (const auto *target = snapshot->Find(link.target))
                            nodes.push_back({document.id.Raw(),link.id.Raw(),link.parent.Raw(),
                                ed::UiElement::PrefabLink,"Linked Prefab · " + target->name,0,0,target->id.Raw()});
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
            out.Number<8>(node.prefab);
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
        const auto *transform_node = SelectedSceneNode(*snapshot);
        constexpr std::string_view transform_names[]{"Local X","Local Y","Local Z","Scale X","Scale Y","Scale Z","Rotation Z (degrees)"};
        out.Number<4>(fields.size() + (transform_node ? std::size(transform_names) : 0));
        if (transform_node) {
            const auto &t = transform_node->local_transform;
            const bool planar = t.rotation.x == 0 && t.rotation.y == 0;
            const double values[]{t.position.x,t.position.y,t.position.z,t.scale.x,t.scale.y,t.scale.z,
                2.0 * std::atan2(t.rotation.z,t.rotation.w) * 180.0 / 3.141592653589793};
            for (std::size_t index = 0; index < std::size(values); ++index) {
                const bool supported = index != 6 || planar;
                out.Text("transform:" + std::to_string(index));
                out.Text(transform_names[index]);
                out.Number<1>(static_cast<unsigned>(ed::comp::PropertyKind::Real));
                out.Text(supported ? ed::EditorWorkspace::FormatValue(ed::comp::PropertyValue{values[index]}) : "Non-planar rotation");
                out.Number<1>(supported && !(workspace_->Session() && workspace_->Session()->IsPlaying()));
                out.Text("Scene local transform");
                out.Text(supported ? "" : "Edit Z rotation on XY nodes");
                out.Number<8>(0);
            }
        }
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
        if (projected_source_.lock() != snapshot) {
            std::map<std::uint64_t,ed::SceneView> projections;
            for (const auto &doc : snapshot->definition.documents)
                if (doc.Kind() == ed::DocumentKind::Scene)
                    Require(projections[doc.id.Raw()].Update(*snapshot,doc.id));
            scene_projections_.swap(projections);
            projected_source_ = snapshot;
        }
        const bool dragging = drag_.Current(*workspace_);
        const auto &scenes = scene_projections_;
        const auto &projection_for = [&](std::uint64_t document) -> const ed::SceneView & {
            return dragging && drag_view_.Document().Raw() == document ? drag_view_ : scenes.at(document);
        };
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
                    const auto &projection = projection_for(doc.id.Raw());
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
        const auto types = schema_.components.Types();
        out.Number<4>(types.size());
        for (const auto &type : types) {
            out.Number<8>(type.id.Raw());
            out.Text(type.name);
        }
        std::size_t scene_item_count = 0;
        for (const auto &scene : scenes)
            scene_item_count += projection_for(scene.first).Items().size();
        out.Number<4>(scene_item_count);
        for (const auto &scene : scenes)
            for (const auto &item : projection_for(scene.first).Items()) {
                out.Number<8>(scene.first);
                out.Number<8>(item.id.Raw());
                const double values[]{item.bounds.left,item.bounds.top,item.bounds.right,item.bounds.bottom,
                                      item.world.position.x,item.world.position.y,
                                      2.0 * std::atan2(item.world.rotation.z,item.world.rotation.w)};
                for (const double value : values)
                    out.Number<8>(std::bit_cast<std::uint64_t>(value));
            }
        out.Number<4>(snapshot->definition.systems.size());
        for (const auto &system : snapshot->definition.systems) {
            out.Number<8>(system.id.Raw());
            out.Number<8>(system.scene.Raw());
            out.Number<8>(system.graph.Raw());
            out.Number<1>(system.enabled);
        }
        const auto now = std::chrono::steady_clock::now();
        if (folder && now >= refresh_content_folders_) {
            content_folders_ = folder->ContentFolders();
            refresh_content_folders_ = now + std::chrono::seconds{2};
        }
        out.Number<4>(folder ? content_folders_.size() : 0);
        if (folder)
            for (const auto &path : content_folders_)
                out.Text(path);
        out.Number<2>(1); // Graph DTO schema version, independent of IPC envelope.
        out.Number<4>(std::count_if(snapshot->definition.documents.begin(), snapshot->definition.documents.end(),
            [](const auto &doc) { return doc.Kind() == ed::DocumentKind::Graph; }));
        for (const auto &doc : snapshot->definition.documents) {
            const auto *graph = std::get_if<rt::graph::GraphDefinition>(&doc.data);
            if (!graph) continue;
            out.Number<8>(doc.id.Raw());
            out.Number<4>(graph->nodes.size());
            for (std::size_t index = 0; index < graph->nodes.size(); ++index) {
                const auto &node = graph->nodes[index];
                out.Number<8>(node.id.Raw()); out.Number<8>(node.type_id.Raw()); out.Text(node.name);
                const auto position = std::find_if(doc.graph_layout.begin(), doc.graph_layout.end(),
                    [&](const auto &p) { return p.node == node.id.Raw(); });
                const double x = position == doc.graph_layout.end() ? 40.0 + 280.0 * static_cast<double>(index % 4) : position->x;
                const double y = position == doc.graph_layout.end() ? 80.0 + 240.0 * std::floor(static_cast<double>(index) / 4.0) : position->y;
                out.Number<8>(std::bit_cast<std::uint64_t>(x)); out.Number<8>(std::bit_cast<std::uint64_t>(y));
                out.Number<4>(node.ports.size());
                for (const auto &port : node.ports) {
                    out.Number<8>(port.id.Raw()); out.Text(port.name);
                    out.Number<1>(static_cast<unsigned>(port.kind)); out.Number<1>(static_cast<unsigned>(port.direction));
                    out.Number<8>(port.type_id.Raw());
                }
            }
            out.Number<4>(graph->connections.size());
            for (const auto &edge : graph->connections) {
                out.Number<8>(edge.id.Raw()); out.Number<8>(edge.output_port.Raw()); out.Number<8>(edge.input_port.Raw());
            }
        }
    }

  public:
    Backend() {
        Reset();
    }

    bool Request(const Bytes &bytes) {
        std::uint64_t correlation = 0, operation = 0;
        std::string error;
        bool quit = false;
        ed::DecodedPng preview;
        const auto previous_selection = workspace_->Selection();
        const auto previous_selection_revision = selection_revision_;
        try {
            Reader in{bytes};
            if (in.Number(4) != Magic || in.Number(2) != 4 || in.Number(2) != 0)
                throw std::runtime_error("ui.protocol_version");
            correlation = in.Number(8);
            operation = in.Number(1);
            const auto generation = in.Number(8), revision = in.Number(8);
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
            if ((operation >= 5 && operation <= 9) || (operation >= 12 && operation <= 17) ||
                (operation >= 29 && operation <= 31) || (operation == 32 && (args.empty() || args[0] != "cancel")) ||
                (operation >= 33 && operation <= 35))
                CheckWrite({generation, revision});
            if ((operation == 5 || (operation >= 15 && operation <= 24) || (operation >= 29 && operation <= 35)) && workspace_->Session() &&
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
                if (kind > static_cast<unsigned>(ed::UiElement::PrefabLink))
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
                if (args[1].starts_with("transform:")) {
                    const auto snap = workspace_->Project()->Snapshot();
                    const auto *node = SelectedSceneNode(*snap);
                    const auto field = Numeric(std::string_view(args[1]).substr(10));
                    if (!node || field > static_cast<unsigned>(ed::UiTransformField::RotationZDegrees))
                        throw std::runtime_error("ui.field_missing");
                    const auto command = ed::EditorWorkspace::TransformCommand(workspace_->Selection().document,
                        node->id,node->local_transform,static_cast<ed::UiTransformField>(field),args[2]);
                    Require(command);
                    Require(workspace_->Execute({command.Value()}, revision));
                    break;
                }
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
            case 29:
                EditSceneHierarchy(args, revision);
                break;
            case 30:
                EditComponent(args, revision);
                break;
            case 31: {
                if ((args.size() != 1 && args.size() != 2) || Numeric(args[0]) != selection_revision_)
                    throw std::runtime_error("ui.stale_selection");
                if (args.size() == 2) {
                    const auto rows = Inspector();
                    bool found = false;
                    for (std::size_t index = 0; index < rows.size(); ++index)
                        if (FieldKey(rows[index],index) == args[1]) {
                            const auto command = ed::InspectorResetCommand(rows[index]);
                            Require(command);
                            Require(workspace_->Execute({command.Value()},revision));
                            found = true;
                            break;
                        }
                    if (!found)
                        throw std::runtime_error("ui.field_missing");
                    break;
                }
                const auto snap = workspace_->Project()->Snapshot();
                const auto *node = SelectedSceneNode(*snap);
                if (!node)
                    throw std::runtime_error("ui.scene_selection");
                Require(workspace_->Execute({ed::SetSceneTransform{workspace_->Selection().document,node->id,{}}},revision));
                break;
            }
            case 32:
                SceneGesture(args);
                break;
            case 33:
                PlaceResource(args,revision);
                break;
            case 34:
                EditSystems(args,revision);
                break;
            case 35:
                DocumentLifecycle(args, revision);
                break;
            case 36:
                preview = AssetPreview(args);
                break;
            case 38:
                CheckWrite({generation, revision});
                if (workspace_->Session() && workspace_->Session()->IsPlaying())
                    throw std::runtime_error("ui.playing_readonly");
                GraphEdit(args, revision);
                break;
            case 37: {
                CheckWrite({generation, revision});
                const auto *folder = workspace_->Folder();
                if (!folder || args.size() != 1)
                    throw std::runtime_error("ui.content_folder");
                if (workspace_->Session() && workspace_->Session()->IsPlaying())
                    throw std::runtime_error("ui.playing_readonly");
                Require(folder->CreateContentFolder(args.front()));
                refresh_content_folders_ = {};
                break;
            }
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
        out.Number<2>(4);
        out.Number<2>(0);
        out.Number<8>(correlation);
        out.Number<1>(error.empty() ? 0 : 1);
        out.Number<8>(generation_);
        out.Text("snapshot,open,new-folder,rename-document,save,undo,redo,selection,close,clone-"
                 "folder,play,stop,create-document,scalar-inspector,discard-document,prefab-"
                 "default-components,asset-pipeline,scene-hierarchy,component-authoring,scene-gestures,scene-resources,scene-systems,document-lifecycle,asset-preview,content-folders,graph-authoring");
        out.Text(error);
        if (operation == 36) {
            out.Number<4>(preview.width);
            out.Number<4>(preview.height);
        } else
            Snapshot(out);
        Wire prefix;
        prefix.Number<4>(out.data.size() + preview.pixels.size());
        std::cout.write(reinterpret_cast<const char *>(prefix.data.data()), 4);
        std::cout.write(reinterpret_cast<const char *>(out.data.data()),
                        static_cast<std::streamsize>(out.data.size()));
        if (!preview.pixels.empty())
            std::cout.write(reinterpret_cast<const char *>(preview.pixels.data()),
                            static_cast<std::streamsize>(preview.pixels.size()));
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
