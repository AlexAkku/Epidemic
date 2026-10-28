#include "Epidemic/Game/Editor/editor_inspector.h"
#include "../../example/demo_game.h"
#include <algorithm>
#include <cmath>
#include <source_location>

using namespace epidemic;
namespace ed=game::editor;
namespace comp=gameplay::composition;
namespace ex=ed::example;
namespace {
struct UserSettings { std::int64_t lives=5; double strength=2.5; bool enabled=true; std::string caption="user"; };
struct Other {};
bool ValidLives(const std::int64_t &value) { return value>=0 && value<=100; }
bool ValidStrength(const double &value) { return std::isfinite(value) && value>=0 && value<=1000; }
void Check(bool value,const std::source_location at=std::source_location::current()) {
    if (!value) throw std::runtime_error("Inspector invariant at line "+std::to_string(at.line()));
}
comp::PropertyValue Value(const ed::InspectorEntry &entry) {
    if (!entry.value) throw std::runtime_error("Missing Inspector value: "+entry.name);
    return *entry.value;
}
}
void InspectorCases() {
    gameplay::entities::EntityService entities; comp::ComponentRegistry registry; int factories=0;
    gameplay::entities::EntityArchetypeDefinition archetype; archetype.canonical_name="studio.unit";
    const auto archetype_id=ex::Take(entities.RegisterArchetype(archetype)); entities.Freeze();
    const auto type=ex::Take(registry.Register<UserSettings>("studio.user.settings",[&] { ++factories; return std::make_shared<UserSettings>(); }));
    const auto lives=ex::Take(registry.RegisterProperty(type,"lives",&UserSettings::lives,&ValidLives));
    const auto strength=ex::Take(registry.RegisterProperty(type,"strength",&UserSettings::strength,&ValidStrength));
    const auto enabled=ex::Take(registry.RegisterProperty(type,"enabled",&UserSettings::enabled));
    const auto caption=ex::Take(registry.RegisterProperty(type,"caption",&UserSettings::caption));
    const auto other=ex::Take(registry.Register<Other>("studio.other")); registry.Freeze();
    const ed::ProjectSchema schema{entities,registry,{}};
    ed::EditorWorkspace workspace{schema,[] {
        ed::PlayOptions options; options.backend=std::make_unique<ex::Backend>();
        options.bindings=[](ed::PlayContext &,runtime::AssetId) { return std::vector<game::composition::NodeBinding>{}; };
        return options;
    }};
    const runtime::AssetId prefab{ex::Id("inspector.prefab")}, scene{ex::Id("inspector.scene")};
    const auto node=ex::Id("user.node"), parameter=ex::Id("power");
    const comp::PrefabComponent recipe{type,{},{{lives,std::int64_t{5},{}},{strength,2.5,{}},{enabled,true,{}},{caption,std::string{"user"},{}}},{}};
    comp::PrefabDefinition definition{prefab.value,{{parameter,4.5}},{node,archetype_id,{recipe},{},{}}};
    game::composition::SceneCompositionDefinition scene_definition; scene_definition.asset_id=scene;
    runtime::SceneNodeSnapshot placement; placement.id=runtime::SceneNodeId{42}; scene_definition.scene.nodes.push_back(placement);
    scene_definition.placements.push_back({placement.id,prefab,{}});
    const std::filesystem::path file{EPIDEMIC_EDITOR_TEST_DIRECTORY "/inspector.epidemic"};
    ex::Take(workspace.New(file,{ex::Id("inspector.project"),scene,{{scene,"Scene",0,scene_definition},{prefab,"User",0,definition}}}));
    ex::Take(workspace.Save());
    const ed::UiSelection selection{prefab,ed::UiElement::PrefabComponent,node.Raw(),type,{}};
    Check(workspace.Select(selection));
    const auto entries=[&](const ed::UiSelection &selected) { return ed::InspectComponents(*workspace.Project()->Snapshot(),selected,registry,workspace.Session()); };
    const auto property=[&](comp::PropertyId id) {
        for (const auto &row:entries(selection)) if (row.property==id) return row;
        throw std::runtime_error("Missing schema field");
    };
    Check(entries(selection).size()==5 && factories==0); // Four fields plus the definition parameter.
    const auto apply=[&](comp::PropertyId id,std::string_view text) {
        ex::Take(workspace.Execute({ex::Take(ed::InspectorCommand(property(id),text))},workspace.Project()->Revision()));
    };
    apply(lives,"42"); apply(strength,"3.75"); apply(enabled,"false"); apply(caption,"  Unicode: Привет  ");
    Check(std::get<std::int64_t>(Value(property(lives)))==42 && std::get<double>(Value(property(strength)))==3.75);
    Check(!std::get<bool>(Value(property(enabled))) && std::get<std::string>(Value(property(caption)))=="  Unicode: Привет  ");
    const auto checkpoint=workspace.Project()->Snapshot(); const auto epoch=workspace.Project()->Revision();
    for (const auto text:{"nan","inf","-inf","1e999"}) Check(!ed::InspectorCommand(property(strength),text));
    Check(!ed::InspectorCommand(property(lives),"9223372036854775808")); Check(!ed::InspectorCommand(property(enabled),"1"));
    const auto invalid=ex::Take(ed::InspectorCommand(property(lives),"-1"));
    Check(!workspace.Execute({ex::Take(ed::InspectorCommand(property(caption),"must roll back")),invalid},epoch));
    Check(workspace.Project()->Snapshot()==checkpoint && workspace.Project()->Revision()==epoch && factories==0);
    ex::Take(workspace.Save()); apply(lives,"43"); ex::Take(workspace.Undo()); Check(!workspace.Project()->IsDirty());
    ex::Take(workspace.Redo()); ex::Take(workspace.Save()); ex::Take(workspace.Open(file)); Check(workspace.Select(selection));
    Check(std::get<std::int64_t>(Value(property(lives)))==43 && factories==0);
    ex::Take(workspace.Execute({ed::SetProperty{prefab,node,type,{}, {strength,999.,parameter}}},workspace.Project()->Revision()));
    Check(std::get<double>(Value(property(strength)))==4.5 && !property(strength).editable);
    Check(!ed::InspectorCommand(property(strength),"6"));
    auto placed=scene_definition.placements[0]; placed.overrides.parameters.push_back({parameter,7.5});
    ex::Take(workspace.Execute({ed::PlacePrefab{scene,placed}},workspace.Project()->Revision()));
    const ed::UiSelection scene_selection{scene,ed::UiElement::SceneNode,42};
    const auto effective=[&] {
        for (const auto &row:entries(scene_selection)) if (row.property==strength && row.source!="runtime snapshot") return row;
        throw std::runtime_error("Missing effective field");
    };
    Check(std::get<double>(Value(effective()))==7.5 && effective().source.starts_with("parameter override"));
    placed.overrides.components.push_back({node,type,{}, {strength,8.5}});
    ex::Take(workspace.Execute({ed::PlacePrefab{scene,placed}},workspace.Project()->Revision()));
    Check(std::get<double>(Value(effective()))==8.5 && effective().source=="instance override" && !effective().editable);
    ex::Take(workspace.Play()); Check(factories==1);
    Check(ex::Take(workspace.Session()->ReadComponentProperty(placement.id,node,type,{},strength))==comp::PropertyValue{8.5});
    const auto detached=ex::Take(workspace.Session()->ReadComponentProperty(placement.id,node,type,{},caption));
    const auto runtime_rows=entries(scene_selection);
    Check(std::any_of(runtime_rows.begin(),runtime_rows.end(),[](const auto &row) { return row.source=="runtime snapshot" && !row.editable; }));
    apply(lives,"50"); Check(ex::Take(workspace.Session()->ReadComponentProperty(placement.id,node,type,{},lives))==comp::PropertyValue{std::int64_t{43}});
    ex::Take(workspace.Stop()); Check(!workspace.Session()->ReadComponentProperty(placement.id,node,type,{},lives));
    Check(std::get<std::string>(detached)=="  Unicode: Привет  ");
    // Add/replace uses one atomic command transaction; collisions cannot overwrite slots.
    const ed::UiSelection entity_selection{prefab,ed::UiElement::PrefabNode,node.Raw()};
    Check(!ed::ComponentCommands(*workspace.Project()->Snapshot(),entity_selection,recipe));
    const auto referenced=workspace.Project()->Snapshot();
    auto commands=ex::Take(ed::ComponentCommands(*referenced,selection,{other,{},{},{}},true));
    Check(!workspace.Execute(commands,workspace.Project()->Revision()) && workspace.Project()->Snapshot()==referenced);
    // Existing instance overrides must not become dangling component references.
    placed.overrides={}; ex::Take(workspace.Execute({ed::PlacePrefab{scene,placed}},workspace.Project()->Revision()));
    const auto before_replace=workspace.Project()->Snapshot();
    commands=ex::Take(ed::ComponentCommands(*before_replace,selection,{other,{},{},{}},true));
    Check(commands.size()==2); ex::Take(workspace.Execute(commands,workspace.Project()->Revision()));
    ex::Take(workspace.Undo()); Check(workspace.Project()->Snapshot()==before_replace);
    auto unknown=recipe; unknown.type=gameplay::TypeId::FromString("unregistered");
    const auto rejected=ex::Take(ed::ComponentCommands(*before_replace,selection,unknown,true));
    Check(!workspace.Execute(rejected,workspace.Project()->Revision()) && workspace.Project()->Snapshot()==before_replace);
    // Omitted defaults stay unknown; no component factory is called to invent a value.
    auto omitted=recipe; omitted.properties.erase(omitted.properties.begin()+1);
    ex::Take(workspace.Execute({ed::SetComponent{prefab,node,omitted}},workspace.Project()->Revision()));
    Check(!property(strength).value && property(strength).editable && property(strength).source=="factory default unavailable");
    const auto count=factories; apply(strength,"2.25"); Check(factories==count);
    Check(!ed::InspectorCommand(property(caption),"line one\nline two"));
    ex::Take(workspace.Execute({ed::SetProperty{prefab,node,type,{}, {caption,std::string{"line one\nline two"},{}}}},workspace.Project()->Revision()));
    Check(!property(caption).editable && ed::InspectorText(property(caption)).find("\\n")!=std::string::npos);
}
