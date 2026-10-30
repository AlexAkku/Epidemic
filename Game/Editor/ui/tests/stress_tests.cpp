#include "Epidemic/Game/Editor/editor_inspector.h"
#include "Epidemic/Game/Editor/scene_drag.h"
#include "../../example/editor_stress.h"
#include "../../src/project_archive.h"
#include <algorithm>
#include <chrono>
#include <cstdio>
#include <limits>
#include <source_location>

using namespace epidemic;
namespace ed=game::editor;
namespace ex=ed::example;
namespace comp=gameplay::composition;
namespace {
void Check(bool value,const std::source_location at=std::source_location::current()) {
    if (!value) throw std::runtime_error("Editor stress invariant at line "+std::to_string(at.line()));
}
template<class F> double Measure(std::size_t count,const char *name,F &&action,int repeats=1) {
    const auto start=std::chrono::steady_clock::now();
    for (int i=0;i<repeats;++i) action();
    const double ms=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-start).count()/repeats;
    std::printf("M8 placements=%zu metric=%s value=%.6f\n",count,name,ms); return ms;
}
void Run(std::size_t count) {
    ex::Schema schema; const auto scene=runtime::AssetId::FromString("example.scene"),prefab=runtime::AssetId::FromString("example.prefab");
    const runtime::SceneNodeId first{42}; const ed::UiSelection selected{scene,ed::UiElement::SceneNode,42};
    const auto file=std::filesystem::path{EPIDEMIC_EDITOR_TEST_DIRECTORY}/("stress-"+std::to_string(count)+".epidemic");
    std::filesystem::create_directories(file.parent_path());
    std::uint64_t effects=0;
    ed::EditorWorkspace workspace{schema.View(),[&]{
        auto options=ex::Options(schema);
        const auto factory=std::make_shared<decltype(options.bindings)>(options.bindings);
        options.bindings=[factory,&effects](ed::PlayContext &context,runtime::AssetId asset){
            auto bindings=(*factory)(context,asset);
            for(auto &binding:bindings) if(binding.invoke) {
                const auto invoke=std::make_shared<game::composition::DomainOperation>(binding.invoke);
                binding.invoke=[invoke,&effects](const game::composition::OperationContext &operation,std::int64_t amount){
                    ++effects; return (*invoke)(operation,amount);
                };
            }
            return bindings;
        };
        return options;
    }};
    Measure(count,"create_ms",[&]{ex::Take(workspace.New(file,ex::Take(ex::StressDefinition(file,schema.View(),count))));});
    Check(workspace.Select(selected));
    Measure(count,"save_ms",[&]{ex::Take(workspace.Save());});
    ed::SceneView view; view.Resize(1000,700);
    Measure(count,"projection_ms",[&]{ex::Take(view.Update(*workspace.Project()->Snapshot(),scene));},3); view.Fit();
    Check(view.Items().size()==count);
    std::uint64_t picked=0;
    Measure(count,"pick_ms",[&]{picked=view.Pick(view.ToScreen({-6,0})).Raw();},1000); Check(picked!=0);
    const auto source=workspace.Project()->Snapshot(); const auto revision=workspace.Project()->Revision();
    ed::SceneDrag drag; const auto start=view.ToScreen({-6,0});
    Measure(count,"drag_begin_ms",[&]{ex::Take(drag.Begin(workspace,view,ed::GizmoTool::Move,ed::GizmoSpace::World,ed::GizmoHandle::X,start));});
    int moves=0;
    Measure(count,"drag_preview_ms",[&]{++moves; ex::Take(drag.Move(view,{start.x+view.Scale()*(1+moves/60.),start.y},1));},60);
    Check(workspace.Project()->Snapshot()==source && workspace.Project()->Revision()==revision && !workspace.Project()->IsDirty());
    Measure(count,"drag_commit_ms",[&]{Check(ex::Take(drag.Commit(workspace)));});
    Measure(count,"undo_ms",[&]{ex::Take(workspace.Undo());}); Check(workspace.Project()->Snapshot()==source && !workspace.Project()->IsDirty());
    Measure(count,"redo_ms",[&]{ex::Take(workspace.Redo());}); ex::Take(workspace.Undo());
    const auto apply=[&](std::vector<ed::Command> commands){ex::Take(workspace.Execute(std::move(commands),workspace.Project()->Revision()));};
    const auto rejects=[&](std::vector<ed::Command> commands){
        const auto before=workspace.Project()->Snapshot(); const auto epoch=workspace.Project()->Revision(); const bool redo=workspace.Project()->CanRedo();
        Check(!workspace.Execute(std::move(commands),epoch));
        Check(workspace.Project()->Snapshot()==before && workspace.Project()->Revision()==epoch && workspace.Project()->CanRedo()==redo && !workspace.Project()->IsDirty());
    };
    rejects({ed::RenameDocument{scene,"must roll back"},ed::SetComponentOverride{scene,first,prefab,{ex::Id("enemy"),schema.health,{}, {schema.points,std::string{"wrong"}}}}});
    rejects({ed::SetComponentOverride{scene,first,prefab,{ex::Id("enemy"),schema.movement,{}, {schema.speed,std::numeric_limits<double>::infinity()}}}});
    rejects({ed::SetSceneParent{scene,first,runtime::SceneNodeId{99999}}}); rejects({ed::SetSceneParent{scene,first,first}});
    rejects({ed::DeleteDocument{prefab}}); rejects({ed::PlacePrefab{scene,{first,runtime::AssetId::FromString("missing"),{}}}});
    runtime::SceneNodeSnapshot duplicate; duplicate.id=first; rejects({ed::AddSceneNode{scene,duplicate}});
    rejects({ed::ConnectPorts{runtime::AssetId::FromString("example.graph"),{ex::Id("invalid.edge"),ex::Id("before.result"),ex::Id("before.in")}}});
    Check(!workspace.Execute({ed::RenameDocument{scene,"stale"}},workspace.Project()->Revision()-1));
    apply({ed::RenameDocument{scene,"Branch"}}); Check(!workspace.Project()->CanRedo());
    ex::Take(workspace.Undo()); Check(workspace.Project()->Snapshot()==source && !workspace.Project()->IsDirty());
    ex::Take(workspace.Redo()); Check(workspace.Select({scene,ed::UiElement::SceneNode,43}));
    apply({ed::DeleteSceneNode{scene,runtime::SceneNodeId{43}}}); Check(!workspace.Select({scene,ed::UiElement::SceneNode,43}));
    ex::Take(workspace.Undo()); Check(workspace.Select({scene,ed::UiElement::SceneNode,43})); ex::Take(workspace.Save());
    apply({ed::RenameDocument{scene,"Unsaved"}});
    Check(!workspace.Open(file) && workspace.Project()->IsDirty()); ex::Take(workspace.Open(file,true));
    Check(!workspace.Project()->IsDirty() && workspace.Selection().kind==ed::UiElement::Document && !workspace.Project()->CanUndo());
    const auto loaded=workspace.Project()->Snapshot(); Check(!workspace.Open(file.parent_path()/"missing.epidemic") && workspace.Project()->Snapshot()==loaded);
    Measure(count,"snapshot_copy_ms",[&]{auto copy=loaded->definition; Check(copy.documents.size()==3);},5);
    auto archive=ex::Take(ed::detail::ReadProject(ex::Take(ed::detail::ReadFile(file)),schema.View()));
    const std::set<std::pair<std::uint64_t,std::uint64_t>> identities{archive.used_ids.begin(),archive.used_ids.end()};
    Measure(count,"archive_preflight_ms",[&]{ex::Take(ed::detail::ValidateForPersistence(archive.definition,identities,schema.View()));},3);
    const int commits=count==200?66:16;
    Measure(count,"history_commit_ms",[&]{static int serial=0; apply({ed::RenameDocument{scene,"History "+std::to_string(++serial)}});},commits);
    int undo=0; while(workspace.Project()->CanUndo()){ex::Take(workspace.Undo()); ++undo;} Check(undo==std::min(commits,64));
    int redo=0; while(workspace.Project()->CanRedo()){ex::Take(workspace.Redo()); ++redo;} Check(redo==undo); ex::Take(workspace.Open(file,true));
    Measure(count,"open_ms",[&]{ex::Take(workspace.Open(file));},3);
    const auto authored=workspace.Project()->Snapshot();
    Check(effects==0);
    const int cycles=count==200?10:3;
    Measure(count,"play_stop_ms",[&]{
        const auto before=effects;
        ex::Take(workspace.Play()); ex::Take(workspace.Send(first,{ex::Id("hit"),1})); ex::Take(workspace.Tick({})); Check(effects==before+1);
        const auto handle=workspace.Activations().back(); Check(!handle->IsFinished());
        ex::Take(workspace.Stop()); const auto report=handle->Report(); Check(handle->IsFinished() && report && report->status==runtime::graph::ExecutionStatus::Cancelled);
        ex::Take(workspace.Stop()); ex::Take(workspace.Tick(runtime::GameDuration{1})); Check(effects==before+1);
        Check(!workspace.Session()->ReadComponentProperty(first,ex::Id("enemy"),schema.health,{},schema.points));
        Check(workspace.Project()->Snapshot()==authored && !workspace.Project()->IsDirty());
    },cycles);
    std::puts("PASS: bounded authoring/history/validation/lifecycle stress; timings are observations, not thresholds");
}
}
int main(int argc,char **argv) {
    try {
        if(argc>2 || (argc==2 && std::string_view{argv[1]}!="--large")) throw std::runtime_error("usage: EpidemicEditorStressTests [--large]");
        Run(argc==2?1000:200); return 0;
    } catch(const std::exception &error){std::fprintf(stderr,"%s\n",error.what()); return 1;}
}
