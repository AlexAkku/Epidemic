#include "demo_game.h"
#include "graph_authoring.h"
#include "script_worker.h"
#include <fstream>
#include "Epidemic/Runtime/Support/graph_scheduler.h"
#include <sstream>
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
#include <TlHelp32.h>

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

class PreviewClock final : public rt::simulation::ISimulationClock {
    std::shared_ptr<rt::IGameClock> clock_;
  public:
    explicit PreviewClock(std::shared_ptr<rt::IGameClock> clock) : clock_(std::move(clock)) {}
    rt::simulation::SimulationTime Now() const override {return clock_->Now();}
};

struct Node {
    std::uint64_t document = 0, element = 0, parent = 0;
    ed::UiElement kind = ed::UiElement::Document;
    std::string label;
    std::uint64_t component = 0, part = 0;
    std::uint64_t prefab = 0;
};

class Backend {
    ex::Schema schema_;
    std::vector<ed::graph_ui::Recipe> Recipes() const {
        return workspace_&&workspace_->Project()?ed::graph_ui::Catalogue(const_cast<ex::Schema &>(schema_).View(),workspace_->Project()->Snapshot()->definition):ed::graph_ui::Catalogue(const_cast<ex::Schema &>(schema_).View());
    }
    std::filesystem::path ScriptPath(const std::string &relative) const {
        const auto root=std::filesystem::weakly_canonical(workspace_->Folder()?workspace_->Folder()->Root():workspace_->Project()->File().parent_path());
        const auto path=std::filesystem::weakly_canonical(root/Path(relative));
        const auto rel=path.lexically_relative(root);
        if(relative.empty()||rel.empty()||rel.is_absolute()||*rel.begin()==L"..")throw std::runtime_error("script.path_outside_project");
        return path;
    }
    std::filesystem::path WorkerPath() const {
        wchar_t path[32768];const auto count=GetModuleFileNameW(nullptr,path,32768);
        if(!count||count==32768)throw std::runtime_error("script.worker_path");
        return std::filesystem::path(path).parent_path()/L"managed/Epidemic.ScriptWorker.dll";
    }
    std::weak_ptr<ed::scripts::SharedWorker> active_script_worker_;
    void StopScripts(){if(auto worker=active_script_worker_.lock())worker->Stop();active_script_worker_.reset();}
    std::uint64_t script_generation_=0;
    ed::PlayOptions ScriptPlayOptions() {
        const auto play_generation=++script_generation_;
        auto options=ex::Options(schema_);auto original=std::make_shared<const decltype(options.bindings)>(options.bindings);
        auto descriptors=std::make_shared<const std::vector<ed::UserSchema>>(workspace_->Project()->Snapshot()->definition.user_schemas);
        auto shared=descriptors->empty()?std::shared_ptr<ed::scripts::SharedWorker>{}:std::make_shared<ed::scripts::SharedWorker>(WorkerPath(),ScriptPath(descriptors->front().assembly),descriptors->front().fingerprint,play_generation);
        active_script_worker_=shared;
        options.bindings=[original,descriptors,shared](ed::PlayContext &context,rt::AssetId graph){
            auto bindings=(*original)(context,graph);
            for(const auto &type:*descriptors)for(const auto &operation:type.operations){
                bindings.push_back({Id::FromString(operation.id),rt::graph::Operation::NativeCall,
                    [shared,type=std::shared_ptr<const ed::UserSchema>(descriptors,&type),operation=std::shared_ptr<const ed::UserOperation>(descriptors,&operation),runtime=&context](const epidemic::game::composition::OperationContext &call,std::int64_t argument){
                        using Result=epidemic::foundation::Result<std::int64_t>;
                        try {
                            if(!call.scope.IsAlive())throw std::runtime_error("script.stale_scope");
                            const auto handle=call.scope.Handle();const auto key=std::to_string(handle.slot)+":"+std::to_string(handle.generation)+":"+std::to_string(call.scope.SystemId().Raw())+":"+std::to_string(call.scope.Part().Raw());
                            ed::scripts::Worker::Fields fields;
                            const auto entity=runtime->entities.Resolve(handle);
                            const auto component=entity?runtime->components.Find(entity->id,epidemic::gameplay::TypeId::FromString(type->component.name),call.scope.Part()):std::nullopt;
                            if(entity&&!component)throw std::runtime_error("script.component_missing");
                            if(component)for(const auto &field:type->component.fields){auto value=runtime->components.ReadProperty(*component,Id::FromString(field.name));Require(value);fields.emplace_back(field.name,value.Value());}
                            auto result=shared->Invoke(operation->id,type->component.name,key,argument,fields);
                            if(!result)return result;
                            if(component){std::vector<ed::comp::PropertyAssignment> assignments;for(const auto &[name,value]:fields)assignments.push_back({Id::FromString(name),value});
                                Require(ValidateScriptResponse(*type,assignments));
                                for(const auto &assignment:assignments)Require(runtime->components.WriteProperty(*component,assignment));}
                            return result;
                        }catch(const std::exception &error){return Result::Failure(epidemic::foundation::Error::Create("script.runtime",error.what()));}
                    }});
            }
            return bindings;
        };return options;
    }
    static epidemic::foundation::Result<void> ValidateScriptResponse(const ed::UserSchema &type,const std::vector<ed::comp::PropertyAssignment> &fields){
        // Validate the complete response before publishing native live fields.
        if(fields.size()!=type.component.fields.size())return epidemic::foundation::Result<void>::Failure(epidemic::foundation::Error::Create("script.field_count","managed response"));
        std::set<std::uint64_t> ids;
        for(const auto &field:fields){const auto found=std::find_if(type.component.fields.begin(),type.component.fields.end(),[&](const auto &entry){return Id::FromString(entry.name)==field.property;});
            if(found==type.component.fields.end()||!ids.insert(field.property.Raw()).second||found->initial.index()!=field.value.index())return epidemic::foundation::Result<void>::Failure(epidemic::foundation::Error::Create("script.field_type","managed response"));}
        return epidemic::foundation::Result<void>::Success();
    }
    void RegisterScripts(const std::vector<std::string> &args,std::uint64_t revision) {
        if(args.size()!=1)throw std::runtime_error("script.arguments");
        const auto path=ScriptPath(args[0]);
        if(path.extension()!=L".schema"||std::filesystem::file_size(path)>MaxString)throw std::runtime_error("script.schema_limit");
        std::ifstream file(path,std::ios::binary);Bytes data((std::istreambuf_iterator<char>(file)),{});Reader input{data};
        if(input.Number(2)!=1)throw std::runtime_error("script.schema_version");
        const auto assembly=input.Text(),fingerprint=input.Text();
        (void)ScriptPath(assembly);
        if(fingerprint.size()!=64||!std::all_of(fingerprint.begin(),fingerprint.end(),[](char ch){return (ch>='0'&&ch<='9')||(ch>='a'&&ch<='f');}))throw std::runtime_error("script.fingerprint");
        const auto count=input.Number(4);if(count>64)throw std::runtime_error("script.type_limit");
        std::vector<ed::UserSchema> schemas;
        for(std::uint64_t i=0;i<count;++i){
            ed::UserSchema type;type.assembly=assembly;type.fingerprint=fingerprint;
            type.component.name=input.Text();type.component.version=input.Number(8);
            const auto fields=input.Number(4);if(fields>64)throw std::runtime_error("script.field_limit");
            for(std::uint64_t j=0;j<fields;++j){auto name=input.Text();const auto kind=input.Number(1);const auto value=input.Text();if(kind>3)throw std::runtime_error("script.field_type");
                auto parsed=ed::EditorWorkspace::ParseValue(static_cast<ed::comp::PropertyKind>(kind),value);Require(parsed);type.component.fields.push_back({std::move(name),std::move(parsed.Value())});}
            const auto methods=input.Number(4);if(methods>64)throw std::runtime_error("script.method_limit");
            for(std::uint64_t j=0;j<methods;++j){ed::UserOperation operation;operation.id=input.Text();operation.name=input.Text();operation.type=input.Text();operation.method=input.Text();operation.source=input.Text();operation.line=input.Number(8);type.operations.push_back(std::move(operation));}
            schemas.push_back(std::move(type));
        }
        if(input.position!=data.size())throw std::runtime_error("script.trailing_data");
        Require(workspace_->Execute({ed::RegisterUserSchema{{},std::move(schemas)}},revision));
    }
    std::optional<ed::EditorDocument> graph_clipboard_;
    std::string graph_report_;
    std::uint64_t graph_report_document_ = 0, graph_report_revision_ = 0;
    struct ReferencePreview { ed::ReferenceCandidate candidate; std::string reason; };
    std::vector<ReferencePreview> reference_preview_;
    std::uint64_t reference_preview_revision_ = 0;

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
            const auto &schemas=workspace_->Project()->Snapshot()->definition.user_schemas;
            const bool dynamic=std::any_of(schemas.begin(),schemas.end(),[&](const auto &s){return epidemic::gameplay::TypeId::FromString(s.component.name)==recipe.type;});
            if(!dynamic)for (auto &property : defaults.Value())
                recipe.properties.push_back({property.property, std::move(property.value), {}});
            auto commands = ed::ComponentCommands(*workspace_->Project()->Snapshot(), selection, recipe);
            Require(commands);
            Require(workspace_->Execute(std::move(commands).Value(), revision));
            (void)workspace_->Select({selection.document, ed::UiElement::PrefabComponent, selection.element, recipe.type, recipe.part});
        } else if (args[1] == "remove" && (args.size() == 2 || args.size()==3) &&
                   (selection.kind == ed::UiElement::PrefabComponent || selection.kind==ed::UiElement::PrefabNode)) {
            auto type=selection.component;auto part=selection.part;
            if(args.size()==3){auto identity=GraphWords(args[2],2);if(identity.size()!=2)throw std::runtime_error("ui.component_identity");type=epidemic::gameplay::TypeId{Numeric(identity[0])};part=epidemic::gameplay::entities::EntityPartId{Numeric(identity[1])};}
            Require(workspace_->Execute({ed::DeleteComponent{selection.document,node,type,part}},revision));
            (void)workspace_->Select({selection.document,ed::UiElement::PrefabNode,selection.element});
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
        StopScripts();workspace_.reset();Require(schema_.components.RegisterDescriptions({}));
        drag_.Cancel();
        reference_preview_.clear();reference_preview_revision_=0;graph_clipboard_.reset(); graph_report_.clear(); graph_report_document_=0; graph_report_revision_=0;
        projected_source_.reset();
        scene_projections_.clear();
        thumbnails_.Clear();
        asset_query_.clear();
        refresh_content_folders_ = {};
        content_folders_.clear();
        workspace_.reset();
        workspace_ = std::make_unique<ed::EditorWorkspace>(schema_.View(),
                                                           [this] { return ScriptPlayOptions(); });
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

    static std::vector<std::string> GraphWords(std::string_view text, std::size_t maximum) {
        if (text.size() > 16384) throw std::runtime_error("ui.graph_fragment_limit");
        std::istringstream stream{std::string(text)};
        std::vector<std::string> words;
        for (std::string word; stream >> word;) {
            if (words.size() >= maximum) throw std::runtime_error("ui.graph_fragment_limit");
            words.push_back(std::move(word));
        }
        if (words.empty()) throw std::runtime_error("ui.graph_empty_selection");
        return words;
    }
    void CopyGraph(const ed::EditorDocument &doc, std::string_view ids) {
        const auto words = GraphWords(ids,64);
        std::set<std::uint64_t> selected;
        const auto &graph = std::get<rt::graph::GraphDefinition>(doc.data);
        for (const auto &word : words) {
            const auto id = Numeric(word);
            if (!selected.insert(id).second || std::none_of(graph.nodes.begin(),graph.nodes.end(),[&](const auto &n){return n.id.Raw() == id;}))
                throw std::runtime_error("ui.graph_node");
        }
        auto copy = doc;
        auto &fragment = std::get<rt::graph::GraphDefinition>(copy.data);
        std::erase_if(fragment.nodes,[&](const auto &n){return !selected.contains(n.id.Raw());});
        std::set<std::uint64_t> ports;
        for (const auto &n : fragment.nodes) for (const auto &port : n.ports) ports.insert(port.id.Raw());
        std::erase_if(fragment.connections,[&](const auto &e){return !ports.contains(e.input_port.Raw()) || !ports.contains(e.output_port.Raw());});
        // Resolve implicit v0 grid before filtering, preserving actual source positions.
        copy.graph_layout.clear(); copy.graph_settings.clear();
        for (std::size_t i=0;i<graph.nodes.size();++i) if(selected.contains(graph.nodes[i].id.Raw())) {
            copy.graph_layout.push_back(ed::graph_ui::Position(doc,i));
            copy.graph_settings.push_back(ed::graph_ui::Settings(doc,graph.nodes[i].id.Raw()));
        }
        if (fragment.connections.size() > 128 || fragment.connections.size()+fragment.nodes.size()*3>256) throw std::runtime_error("ui.graph_fragment_limit");
        graph_clipboard_ = std::move(copy);
    }
    std::vector<ed::Command> PasteGraph(rt::AssetId document, double x, double y) {
        if (!graph_clipboard_) throw std::runtime_error("ui.graph_clipboard_empty");
        const auto &source = *graph_clipboard_;
        const auto &graph = std::get<rt::graph::GraphDefinition>(source.data);
        std::vector<ed::Command> commands;
        std::map<std::uint64_t,Id> ports;
        const auto origin = source.graph_layout.front();
        for (std::size_t i=0;i<graph.nodes.size();++i) {
            auto node = graph.nodes[i]; const auto old = node.id.Raw();
            const auto nonce = DocumentNonce(); node.id = Id::FromString("graph.node."+nonce);
            for (std::size_t j=0;j<node.ports.size();++j) {
                const auto id = Id::FromString("graph.port."+nonce+"."+std::to_string(j));
                ports.emplace(node.ports[j].id.Raw(),id); node.ports[j].id = id;
            }
            commands.push_back(ed::AddGraphNode{document,node});
            const auto position = ed::graph_ui::Position(source,i);
            commands.push_back(ed::MoveGraphNode{document,node.id,x+position.x-origin.x,y+position.y-origin.y});
            auto settings = ed::graph_ui::Settings(source,old); settings.node = node.id.Raw();
            commands.push_back(ed::SetGraphNodeSettings{document,node.id,node.name,std::move(settings)});
        }
        for (const auto &edge : graph.connections)
            commands.push_back(ed::ConnectPorts{document,{Id::FromString("graph.edge."+DocumentNonce()),ports.at(edge.output_port.Raw()),ports.at(edge.input_port.Raw())}});
        return commands;
    }
    void PreviewGraph(const ed::EditorDocument &doc, bool execute) {
        namespace g = rt::graph;
        const auto &definition = std::get<g::GraphDefinition>(doc.data);
        graph_report_.clear(); graph_report_document_=doc.id.Raw(); graph_report_revision_=workspace_->Project()->Revision();
        std::vector<g::NodeBinding> bindings;
        std::shared_ptr<ed::scripts::SharedWorker> preview_worker;
        for (const auto &[type,op] : ed::EffectiveOperations(workspace_->Project()->Snapshot()->definition,schema_.View())) {
            if (op == g::Operation::NativeCall) {
                const auto used=std::find_if(definition.nodes.begin(),definition.nodes.end(),[&](const auto &n){return n.type_id==type;});
                bool managed=false;
                for(const auto &descriptor:workspace_->Project()->Snapshot()->definition.user_schemas)for(const auto &operation:descriptor.operations)
                    if(Id::FromString(operation.id)==type){if(!preview_worker)preview_worker=std::make_shared<ed::scripts::SharedWorker>(WorkerPath(),ScriptPath(descriptor.assembly),descriptor.fingerprint,++script_generation_);bindings.push_back({type,op,std::make_shared<ed::scripts::Factory>(preview_worker,operation.id,descriptor.component.name)});managed=true;}
                if (!managed&&used!=definition.nodes.end()) {graph_report_="ui.graph.native_preview_requires_play node="+std::to_string(used->id.Raw());return;}
            } else bindings.push_back({type,op,{},op==g::Operation::Reference?std::make_shared<ed::graph_ui::SnapshotReferenceResolver>(workspace_->Project()->Snapshot()):nullptr});
        }
        const auto program = g::CompileGraph(definition,bindings);
        if (!program) {graph_report_=program.GetError().code+" "+program.GetError().context;return;}
        if (!execute) {graph_report_="OK compiled native VM; instructions="+std::to_string(program.Value()->Instructions().size());return;}
        const auto recipes=Recipes();
        const auto event=std::find_if(definition.nodes.begin(),definition.nodes.end(),[&](const auto &node){
            return std::any_of(recipes.begin(),recipes.end(),[&](const auto &r){return r.node.type_id==node.type_id&&r.operation==g::Operation::EventEntry;});
        });
        const bool entry=std::any_of(definition.nodes.begin(),definition.nodes.end(),[&](const auto &node){
            return std::any_of(recipes.begin(),recipes.end(),[&](const auto &r){return r.node.type_id==node.type_id&&r.operation==g::Operation::Entry;});
        });
        auto instance=event!=definition.nodes.end()&&!entry ? g::GraphInstance::CreateForEvent(program.Value(),event->ports.front().id,ed::graph_ui::Settings(doc,event->id.Raw()).preview_default) : g::GraphInstance::Create(program.Value());
        if(!instance){graph_report_=instance.GetError().code;return;}
        for(const auto &node:definition.nodes){
            const auto recipe=std::find_if(recipes.begin(),recipes.end(),[&](const auto &r){return r.node.type_id==node.type_id;});
            if(recipe!=recipes.end()&&recipe->operation==g::Operation::Input)
                Require(instance.Value()->SetInput(node.ports.front().id,ed::graph_ui::Settings(doc,node.id.Raw()).preview_default));
        }
        // Reuse the engine scheduler and Time authority in an isolated headless preview.
        // Only GraphTask acknowledges Wait after its scheduled game-time deadline.
        auto time=rt::CreateTimeServices();Require(time);
        rt::simulation::SimulationDependencies dependencies;dependencies.clock=std::make_shared<PreviewClock>(time.Value().clock);
        auto simulation=rt::simulation::CreateSimulationServices({},dependencies);Require(simulation);
        simulation.Value().scheduler->SetBudget({1,256});
        rt::GraphScheduler scheduler{time.Value().clock,simulation.Value(),rt::SimulationZoneId{1}};
        auto task=scheduler.Submit(std::move(instance.Value()),{{256,64},{}});Require(task);
        std::size_t instructions=0,steps=0;
        for(;steps<256&&instructions<4096;++steps){
            Require(scheduler.Prepare());const auto tick=simulation.Value().runtime->Tick();Require(tick);

            const auto &last=task.Value()->LastReport();if(!last)throw std::runtime_error("ui.graph.preview_not_scheduled");
            instructions+=last->instructions;
            if(last->status==g::ExecutionStatus::Completed||last->status==g::ExecutionStatus::Failed)break;
            const auto due=task.Value()->NextWake();const auto now=time.Value().clock->Now();
            if(due&&*due>now)Require(time.Value().runtime->Skip(rt::GameDuration{due->ticks-now.ticks}));
        }
        const auto &final=task.Value()->LastReport();
        if(!final)throw std::runtime_error("ui.graph.preview_not_scheduled");
        const auto &report=*final;
        graph_report_="OK native VM "+std::string(report.status==g::ExecutionStatus::Completed?"Completed":"Preview budget reached")+" instructions="+std::to_string(instructions)+" game_ticks="+std::to_string(time.Value().clock->Now().ticks);
        for(const auto &node:definition.nodes)for(const auto &port:node.ports)if(port.kind==g::PortKind::Data){
            const auto value=task.Value()->ReadValue(port.id);
            if(value)graph_report_+="\nnode="+std::to_string(node.id.Raw())+" "+port.name+"="+std::to_string(value.Value());
        }
        if(report.status==g::ExecutionStatus::Failed&&report.diagnostic)
            graph_report_=report.diagnostic->error.code+" node="+std::to_string(report.diagnostic->node.Raw());
        if(task.Value()->NextWake())Require(scheduler.Cancel(*task.Value()));
    }

    void GraphEdit(const std::vector<std::string> &args, std::uint64_t revision) {
        namespace graph = rt::graph;
        if (args.size() < 2) throw std::runtime_error("ui.graph_arguments");
        const auto document = Asset(Numeric(args[1]));
        const auto snapshot = workspace_->Project()->Snapshot();
        const auto *doc = snapshot->Find(document);
        if (!doc || doc->Kind() != ed::DocumentKind::Graph) throw std::runtime_error("ui.graph_document");
        std::vector<ed::Command> commands;
        if (args[0] == "create" && args.size() == 5) {
            const auto recipes = Recipes();
            const auto index = Numeric(args[2]);
            if (index >= recipes.size()) throw std::runtime_error("ui.graph_template");
            auto node = recipes[static_cast<std::size_t>(index)].node;
            const auto nonce = DocumentNonce(); node.id = Id::FromString("graph.node." + nonce);
            for (std::size_t i=0;i<node.ports.size();++i) node.ports[i].id=Id::FromString("graph.port."+nonce+"."+std::to_string(i));
            commands.push_back(ed::AddGraphNode{document,node});
            commands.push_back(ed::MoveGraphNode{document,node.id,Real(args[3]),Real(args[4])});
        } else if (args[0] == "connect" && args.size() == 4) {
            commands.push_back(ed::ConnectPorts{document,{Id::FromString("graph.edge."+DocumentNonce()),Id{Numeric(args[2])},Id{Numeric(args[3])}}});
        } else if(args[0]=="reconnect" && args.size()==5){
            commands.push_back(ed::DisconnectPorts{document,Id{Numeric(args[2])}});
            commands.push_back(ed::ConnectPorts{document,{Id::FromString("graph.edge."+DocumentNonce()),Id{Numeric(args[3])},Id{Numeric(args[4])}}});
        } else if (args[0] == "move" && args.size() == 5) {
            commands.push_back(ed::MoveGraphNode{document,Id{Numeric(args[2])},Real(args[3]),Real(args[4])});
        } else if (args[0] == "moves" && args.size() == 3) {
            const auto words=GraphWords(args[2],192); std::set<std::uint64_t> seen;
            if(words.size()%3) throw std::runtime_error("ui.graph_arguments");
            for(std::size_t i=0;i<words.size();i+=3) {
                const auto id=Numeric(words[i]); if(!seen.insert(id).second) throw std::runtime_error("ui.graph_node");
                commands.push_back(ed::MoveGraphNode{document,Id{id},Real(words[i+1]),Real(words[i+2])});
            }
        } else if ((args[0]=="delete" || args[0]=="disconnect") && args.size()==3) {
            if(args[0]=="delete") commands.push_back(ed::DeleteGraphNode{document,Id{Numeric(args[2])}});
            else commands.push_back(ed::DisconnectPorts{document,Id{Numeric(args[2])}});
        } else if (args[0]=="deletebulk" && args.size()==3) {
            for(const auto &word:GraphWords(args[2],64)) commands.push_back(ed::DeleteGraphNode{document,Id{Numeric(word)}});
        } else if ((args[0]=="literal"||args[0]=="inputvalue") && args.size()==4) {
            const auto value=ed::EditorWorkspace::ParseValue(ed::comp::PropertyKind::Integer,args[3]);Require(value);
            const auto number=std::get<std::int64_t>(value.Value());
            if(args[0]=="literal")commands.push_back(ed::SetGraphPortDefault{document,Id{Numeric(args[2])},number});
            else {auto node=std::find_if(std::get<graph::GraphDefinition>(doc->data).nodes.begin(),std::get<graph::GraphDefinition>(doc->data).nodes.end(),[&](const auto &n){return n.id.Raw()==Numeric(args[2]);});
                if(node==std::get<graph::GraphDefinition>(doc->data).nodes.end())throw std::runtime_error("ui.graph_node");
                auto settings=ed::graph_ui::Settings(*doc,node->id.Raw());settings.preview_default=number;commands.push_back(ed::SetGraphNodeSettings{document,node->id,node->name,settings});}
        } else if (args[0]=="settings" && args.size()==8) {
            const auto id=Numeric(args[2]); const auto flags=Numeric(args[7]);
            if(flags>3) throw std::runtime_error("ui.graph_flags");
            const auto value=ed::EditorWorkspace::ParseValue(ed::comp::PropertyKind::Integer,args[4]); Require(value);
            commands.push_back(ed::SetGraphNodeSettings{document,Id{id},args[3],{id,std::get<std::int64_t>(value.Value()),args[5],args[6],(flags&1u)!=0,(flags&2u)!=0}});
        } else if (args[0]=="group" && args.size()==4) {
            const auto &graph=std::get<graph::GraphDefinition>(doc->data);
            for(const auto &word:GraphWords(args[2],64)) {
                const auto id=Numeric(word); const auto n=std::find_if(graph.nodes.begin(),graph.nodes.end(),[&](const auto &node){return node.id.Raw()==id;});
                if(n==graph.nodes.end()) throw std::runtime_error("ui.graph_node");
                auto settings=ed::graph_ui::Settings(*doc,id); settings.group=args[3];
                commands.push_back(ed::SetGraphNodeSettings{document,n->id,n->name,std::move(settings)});
            }
        } else if(args[0]=="cut" && args.size()==3) {
            CopyGraph(*doc,args[2]);for(const auto &word:GraphWords(args[2],64))commands.push_back(ed::DeleteGraphNode{document,Id{Numeric(word)}});
        } else if(args[0]=="copy" && args.size()==3) {CopyGraph(*doc,args[2]);return;
        } else if(args[0]=="paste" && args.size()==4) {commands=PasteGraph(document,Real(args[2]),Real(args[3]));
        } else if(args[0]=="duplicate" && args.size()==3) {
            CopyGraph(*doc,args[2]);
            if(!graph_clipboard_)throw std::runtime_error("ui.graph_clipboard_empty");
            if(graph_clipboard_->graph_layout.empty())throw std::runtime_error("ui.graph_clipboard_empty");
            const auto origin=graph_clipboard_->graph_layout.front(); commands=PasteGraph(document,origin.x+40,origin.y+40);
        } else if((args[0]=="compile" || args[0]=="run") && args.size()==2) {PreviewGraph(*doc,args[0]=="run");if(!graph_report_.starts_with("OK")) throw std::runtime_error(graph_report_);return;
        } else if(args[0]=="select" && args.size()==3) {if(!workspace_->Select({document,ed::UiElement::GraphNode,Numeric(args[2])})) throw std::runtime_error("ui.graph_node"); return;
        } else if(args[0]=="uses" && args.size()==2) {
            graph_report_document_=document.Raw();graph_report_revision_=revision;graph_report_="OK Scene system references";
            for(const auto &sys:snapshot->definition.systems) if(sys.graph==document) graph_report_+="\nScene="+std::to_string(sys.scene.Raw())+" System="+std::to_string(sys.id.Raw());
            return;
        } else throw std::runtime_error("ui.graph_arguments");
        const auto edited=workspace_->Execute(std::move(commands),revision);
        if(!edited){
            graph_report_document_=document.Raw();graph_report_revision_=workspace_->Project()->Revision();
            graph_report_=edited.GetError().code+" "+edited.GetError().context;
            if(args[0]=="connect"||args[0]=="reconnect"){
                const auto input=Numeric(args[args[0]=="connect"?3:4]);const auto &graph=std::get<rt::graph::GraphDefinition>(doc->data);
                for(const auto &n:graph.nodes)if(std::any_of(n.ports.begin(),n.ports.end(),[&](const auto &p){return p.id.Raw()==input;}))graph_report_+=" node="+std::to_string(n.id.Raw());
            }else if(args[0]=="settings"||args[0]=="move"||args[0]=="delete")graph_report_+=" node="+args[2];
            throw std::runtime_error(graph_report_);
        }
        if(args[0]=="create"||args[0]=="paste"||args[0]=="duplicate") {
            const auto *updated=workspace_->Project()->Snapshot()->Find(document);
            const auto &nodes=std::get<graph::GraphDefinition>(updated->data).nodes;
            if(!nodes.empty())(void)workspace_->Select({document,ed::UiElement::GraphNode,nodes.back().id.Raw()});
        } else if(args[0]=="move"||args[0]=="moves") {
            const auto words=GraphWords(args[2],192);(void)workspace_->Select({document,ed::UiElement::GraphNode,Numeric(words.front())});
        }
    }

    std::string ReferenceRejection(const ed::ProjectDefinition &project, rt::AssetId document,
                                   const rt::graph::GraphReference &reference, Id target) const {
        namespace g=rt::graph;
        const auto scope=ed::CheckGraphReferenceScope(project,document,reference);
        if(!scope)return scope.GetError().code;
        const auto description=ed::DescribeGraphReference(project,reference);
        if(!description.available)return description.reason;
        if(!target.IsValid())return {};
        const auto doc=std::find_if(project.documents.begin(),project.documents.end(),[&](const auto &d){return d.id==document;});
        if(doc==project.documents.end()||doc->Kind()!=ed::DocumentKind::Graph)return "ui.graph_document";
        const auto &graph=std::get<g::GraphDefinition>(doc->data);
        for(const auto &node:graph.nodes)for(const auto &port:node.ports)if(port.id==target) {
            if(port.kind!=g::PortKind::Data||port.direction!=g::PortDirection::Input)return "graph.reference_input_required";
            if(port.type_id!=g::ReferenceValueType(reference))return "graph.connection_type";
            if(std::any_of(graph.connections.begin(),graph.connections.end(),[&](const auto &e){return e.input_port==target;}))return "graph.data_input_connected";
            return {};
        }
        return "graph.missing_port";
    }
    void GraphReferenceEdit(const std::vector<std::string> &args,std::uint64_t revision) {
        namespace g=rt::graph;
        if(args.size()<2)throw std::runtime_error("ui.reference_arguments");
        const auto document=Asset(Numeric(args[1]));
        const auto snapshot=workspace_->Project()->Snapshot();
        const auto *doc=snapshot->Find(document);
        if(!doc||doc->Kind()!=ed::DocumentKind::Graph)throw std::runtime_error("ui.graph_document");
        if(args[0]=="probe"&&args.size()==5) {
            reference_preview_.clear();reference_preview_revision_=revision;
            for(auto &candidate:ed::GraphReferenceCandidates(snapshot->definition,Asset(Numeric(args[2])),Id{Numeric(args[3])})) {
                auto reason=ReferenceRejection(snapshot->definition,document,candidate.value,Id{Numeric(args[4])});
                if(candidate.value.kind==g::ReferenceKind::Component)if(const auto *type=schema_.components.Find(epidemic::gameplay::TypeId{candidate.value.component.Raw()}))candidate.label="Get "+type->name+" · "+candidate.label;
                reference_preview_.push_back({std::move(candidate),std::move(reason)});
            }
            return;
        }
        if((args[0]=="jump"||args[0]=="uses")&&args.size()==3) {
            const auto &graph=std::get<g::GraphDefinition>(doc->data);
            const auto found=std::find_if(graph.nodes.begin(),graph.nodes.end(),[&](const auto &n){return n.id.Raw()==Numeric(args[2]);});
            if(found==graph.nodes.end())throw std::runtime_error("graph.invalid_reference");
            const auto reference=found->reference;
            if(!reference.has_value())throw std::runtime_error("graph.invalid_reference");
            const auto &r=reference.value();
            if(args[0]=="jump") {
                if(!ed::DescribeGraphReference(snapshot->definition,r).available)throw std::runtime_error("editor.reference_missing");
                const auto selection=r.kind==g::ReferenceKind::PrefabDefinition?ed::UiSelection{r.document,ed::UiElement::Document,0}:ed::UiSelection{r.document,r.placement.IsValid()?ed::UiElement::SceneNode:ed::UiElement::PrefabNode,r.placement.IsValid()?r.placement.Raw():r.entity.Raw()};
                if(!workspace_->Select(selection))throw std::runtime_error("ui.reference_selection");
            } else {
                graph_report_.clear();graph_report_document_=document.Raw();graph_report_revision_=revision;
                for(const auto &d:snapshot->definition.documents)if(const auto *other=std::get_if<g::GraphDefinition>(&d.data))
                    for(const auto &node:other->nodes)if(node.reference==found->reference)graph_report_+=d.name+" node="+std::to_string(node.id.Raw())+"\n";
            }
            return;
        }
        if(args[0]!="drop"||args.size()!=6)throw std::runtime_error("ui.reference_arguments");
        const auto payload=GraphWords(args[2],7);
        if(payload.size()!=7)throw std::runtime_error("ui.reference_arguments");
        const auto kind=Numeric(payload[0]);
        if(kind>static_cast<unsigned>(g::ReferenceKind::Component))throw std::runtime_error("graph.invalid_reference");
        g::GraphReference reference{static_cast<g::ReferenceKind>(kind),Asset(Numeric(payload[1])),Id{Numeric(payload[2])},Asset(Numeric(payload[3])),Id{Numeric(payload[4])},Id{Numeric(payload[5])},Id{Numeric(payload[6])}};
        const Id target{Numeric(args[3])};
        const auto rejection=ReferenceRejection(snapshot->definition,document,reference,target);
        if(!rejection.empty())throw std::runtime_error(rejection);
        const auto nonce=DocumentNonce();
        const auto id=Id::FromString("graph.reference."+nonce),port=Id::FromString("graph.reference.port."+nonce);
        g::NodeDefinition node{id,g::kReferenceNodeType,"Reference",{{port,"reference",g::PortKind::Data,g::PortDirection::Output,g::ReferenceValueType(reference)}},reference};
        std::vector<ed::Command> edits{ed::AddGraphNode{document,node},ed::MoveGraphNode{document,id,Real(args[4]),Real(args[5])}};
        if(target.IsValid())edits.push_back(ed::ConnectPorts{document,{Id::FromString("graph.reference.edge."+nonce),port,target}});
        Require(workspace_->Execute(std::move(edits),revision));
        (void)workspace_->Select({document,ed::UiElement::GraphNode,id.Raw()});
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
        out.Number<2>(5); // Graph DTO schema version, independent of IPC envelope.
        const auto recipes=Recipes();
        out.Number<4>(recipes.size());
        for(const auto &recipe:recipes) {
            out.Text(recipe.name);out.Text(recipe.category);out.Number<8>(recipe.node.type_id.Raw());out.Number<1>(static_cast<unsigned>(recipe.operation));
            std::string source;std::uint64_t line=1;
            for(const auto &type:snapshot->definition.user_schemas)for(const auto &operation:type.operations)if(Id::FromString(operation.id)==recipe.node.type_id){source=operation.source;line=operation.line;}
            out.Text(source);out.Number<4>(line);
        }
        out.Number<4>(std::count_if(snapshot->definition.documents.begin(), snapshot->definition.documents.end(),
            [](const auto &doc) { return doc.Kind() == ed::DocumentKind::Graph; }));
        for (const auto &doc : snapshot->definition.documents) {
            const auto *graph = std::get_if<rt::graph::GraphDefinition>(&doc.data);
            if (!graph) continue;
            out.Number<8>(doc.id.Raw());
            out.Text(graph_report_document_==doc.id.Raw() && graph_report_revision_==workspace_->Project()->Revision() ? graph_report_ : "");
            out.Number<4>(graph->nodes.size());
            for (std::size_t index = 0; index < graph->nodes.size(); ++index) {
                const auto &node = graph->nodes[index];
                out.Number<8>(node.id.Raw()); out.Number<8>(node.type_id.Raw()); out.Text(node.name);
                const auto position = std::find_if(doc.graph_layout.begin(), doc.graph_layout.end(),
                    [&](const auto &p) { return p.node == node.id.Raw(); });
                const double x = position == doc.graph_layout.end() ? 40.0 + 280.0 * static_cast<double>(index % 4) : position->x;
                const double y = position == doc.graph_layout.end() ? 80.0 + 240.0 * std::floor(static_cast<double>(index) / 4.0) : position->y;
                out.Number<8>(std::bit_cast<std::uint64_t>(x)); out.Number<8>(std::bit_cast<std::uint64_t>(y));
                const auto settings=ed::graph_ui::Settings(doc,node.id.Raw());
                out.Number<8>(std::bit_cast<std::uint64_t>(settings.preview_default));out.Text(settings.comment);out.Text(settings.group);
                out.Number<1>((settings.bookmark ? 1u : 0u) | (settings.breakpoint ? 2u : 0u));
                out.Number<1>(node.reference.has_value());
                if(node.reference) {
                    WriteReference(out,*node.reference);
                    const auto description=ed::DescribeGraphReference(snapshot->definition,*node.reference);
                    out.Number<1>(description.available);out.Text(description.label);out.Text(description.reason);
                }
                out.Number<4>(node.ports.size());
                for (const auto &port : node.ports) {
                    out.Number<8>(port.id.Raw()); out.Text(port.name);
                    out.Number<1>(static_cast<unsigned>(port.kind)); out.Number<1>(static_cast<unsigned>(port.direction));
                    out.Number<8>(port.type_id.Raw());out.Number<1>(port.default_value.has_value());if(port.default_value)out.Number<8>(std::bit_cast<std::uint64_t>(*port.default_value));
                }
            }
            out.Number<4>(graph->connections.size());
            for (const auto &edge : graph->connections) {
                out.Number<8>(edge.id.Raw()); out.Number<8>(edge.output_port.Raw()); out.Number<8>(edge.input_port.Raw());
            }
        }
        out.Number<4>(reference_preview_revision_==workspace_->Project()->Revision()?reference_preview_.size():0);
        if(reference_preview_revision_==workspace_->Project()->Revision())for(const auto &reference_view:reference_preview_) {
            WriteReference(out,reference_view.candidate.value);out.Text(reference_view.candidate.label);out.Text(reference_view.reason);
        }
    }
    static void WriteReference(Wire &out,const rt::graph::GraphReference &r) {
        out.Number<1>(static_cast<unsigned>(r.kind));out.Number<8>(r.document.Raw());out.Number<8>(r.placement.Raw());
        out.Number<8>(r.prefab.Raw());out.Number<8>(r.entity.Raw());out.Number<8>(r.component.Raw());out.Number<8>(r.part.Raw());
    }

  public:
    Backend() {
        Reset();
    }

    void ScriptLifecycleSmoke(const std::filesystem::path &file) {
        Require(workspace_->OpenFolder(file.parent_path()));
        const auto snapshot=workspace_->Project()->Snapshot();
        if(snapshot->definition.user_schemas.empty())throw std::runtime_error("script.smoke_no_schema");
        const auto &type=snapshot->definition.user_schemas.front();
        if(type.operations.empty())throw std::runtime_error("script.smoke_no_operation");
        const auto catalogue=Recipes();
        const auto recipe=[&](std::string_view name){const auto found=std::find_if(catalogue.begin(),catalogue.end(),[&](const auto &entry){return entry.name==name;});if(found==catalogue.end())throw std::runtime_error("script.smoke_recipe");return found->node;};
        auto event=recipe("Event"),call=recipe(type.operations.front().name),output=recipe("Output");
        event.id=Id::FromString("script.smoke.event");call.id=Id::FromString("script.smoke.call");output.id=Id::FromString("script.smoke.output");
        const auto port=[](const auto &node,std::string_view name){const auto found=std::find_if(node.ports.begin(),node.ports.end(),[&](const auto &p){return p.name==name;});if(found==node.ports.end())throw std::runtime_error("script.smoke_port");return found->id;};
        rt::graph::GraphDefinition graph;graph.asset_id=rt::AssetId::FromString("script.smoke.graph."+DocumentNonce());graph.nodes={event,call,output};
        graph.connections={{Id::FromString("smoke.link1"),port(event,"payload"),port(call,"argument")},{Id::FromString("smoke.link2"),port(event,"next"),port(call,"in")},{Id::FromString("smoke.link3"),port(call,"result"),port(output,"value")},{Id::FromString("smoke.link4"),port(call,"next"),port(output,"in")}};
        if(type.operations.size()>1){
            auto second=recipe(type.operations[1].name);second.id=Id::FromString("script.smoke.second");graph.nodes.push_back(second);
            graph.connections[3].input_port=port(second,"in");
            graph.connections[2].output_port=port(second,"result");
            graph.connections.push_back({Id::FromString("smoke.link5"),port(event,"payload"),port(second,"argument")});
            graph.connections.push_back({Id::FromString("smoke.link6"),port(second,"next"),port(output,"in")});
        }
        const auto prefab_id=Id::FromString("script.smoke.prefab."+DocumentNonce()),root=Id::FromString("script.smoke.root");
        ed::comp::PrefabDefinition prefab{prefab_id,{}, {root,schema_.enemy,{{epidemic::gameplay::TypeId::FromString(type.component.name),{},{},{}}},{{Id::FromString("smoke.behavior"),graph.asset_id.value,{{port(event,"event"),port(event,"event")}},{}}},{}}};
        rt::SceneNodeSnapshot placement;placement.id=NewSceneNode();
        const auto scene=snapshot->definition.entry_scene;
        Require(workspace_->Execute({ed::CreateDocument{{graph.asset_id,"Script runtime smoke",0,graph}},ed::CreateDocument{{rt::AssetId{prefab_id},"Script runtime target",0,prefab}},ed::AddSceneNode{scene,placement},ed::PlacePrefab{scene,{placement.id,rt::AssetId{prefab_id},{}}}},workspace_->Project()->Revision()));
        const auto owned_workers=[](){
            HANDLE processes=CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS,0);if(processes==INVALID_HANDLE_VALUE)throw std::runtime_error("script.smoke_process_snapshot");
            PROCESSENTRY32W entry{};entry.dwSize=sizeof(entry);unsigned count=0;
            for(BOOL more=Process32FirstW(processes,&entry);more;more=Process32NextW(processes,&entry))if(entry.th32ParentProcessID==GetCurrentProcessId()&&std::wstring_view(entry.szExeFile)==L"dotnet.exe")++count;
            CloseHandle(processes);return count;
        };
        std::optional<ed::comp::PropertyValue> expected;
        for(unsigned iteration=0;iteration<3;++iteration){
            Require(workspace_->Play());
            const auto before=workspace_->Session()->ReadComponentProperty(placement.id,root,epidemic::gameplay::TypeId::FromString(type.component.name),{},Id::FromString("Health"));Require(before);
            Require(workspace_->Send(placement.id,{port(event,"event"),7}));Require(workspace_->Tick({}));
            if(owned_workers()!=1)throw std::runtime_error("script.smoke_worker_per_generation");
            const auto after=workspace_->Session()->ReadComponentProperty(placement.id,root,epidemic::gameplay::TypeId::FromString(type.component.name),{},Id::FromString("Health"));Require(after);
            if(after.Value()==before.Value())throw std::runtime_error("script.smoke_native_target_unchanged");
            if(expected&&*expected!=after.Value())throw std::runtime_error("script.smoke_generation_state_leak");expected=after.Value();
            const auto activations=workspace_->Activations();if(activations.empty()||!activations.back()->IsFinished())throw std::runtime_error("script.smoke_activation");
            const auto report=activations.back()->Report();if(!report||report->status!=rt::graph::ExecutionStatus::Completed)throw std::runtime_error("script.smoke_runtime_diagnostic");
            auto old=active_script_worker_.lock();Require(workspace_->Stop());StopScripts();
            if(owned_workers()!=0)throw std::runtime_error("script.smoke_orphan_worker");
            if(old){ed::scripts::Worker::Fields fields;if(old->Invoke(type.operations.front().id,type.component.name,"retired",1,fields))throw std::runtime_error("script.smoke_old_handle_accepted");}
        }
        std::cout<<"PASS C# native Play/Send/Tick target mutation, three fresh Play/Stop generations, terminal activations; no authoring Save\n";
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
                Require(workspace_->Stop());StopScripts();
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
            case 40:
                CheckWrite({generation,revision});
                if(workspace_->Session()&&workspace_->Session()->IsPlaying())throw std::runtime_error("ui.playing_readonly");
                GraphReferenceEdit(args,revision);
                break;
            case 39:
                CheckWrite({generation,revision});
                if(workspace_->Session()&&workspace_->Session()->IsPlaying())throw std::runtime_error("ui.playing_readonly");
                RegisterScripts(args,revision);
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
        if(auto worker=active_script_worker_.lock();worker&&worker->Failed()&&workspace_->Session()&&workspace_->Session()->IsPlaying()){
            const auto stopped=workspace_->Stop();StopScripts();if(error.empty())error=stopped?"script.worker_failed_play_stopped":stopped.GetError().code;
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
                 "default-components,asset-pipeline,scene-hierarchy,component-authoring,scene-gestures,scene-resources,scene-systems,document-lifecycle,asset-preview,content-folders,graph-authoring,graph-scene-references");
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

int wmain(int argc,wchar_t **argv) {
    _setmode(_fileno(stdin), _O_BINARY);
    _setmode(_fileno(stdout), _O_BINARY);
    try {
        Backend backend;
        if(argc==3&&std::wstring_view(argv[1])==L"--script-lifecycle-smoke"){backend.ScriptLifecycleSmoke(std::filesystem::path(argv[2]));return 0;}
        if(argc!=1)throw std::runtime_error("ui.arguments");
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
