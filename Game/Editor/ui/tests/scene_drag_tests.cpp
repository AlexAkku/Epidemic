#include "Epidemic/Game/Editor/scene_drag.h"
#include "../../example/demo_game.h"
#include <cmath>
#include <limits>
#include <source_location>

using namespace epidemic;
namespace ed=game::editor;
namespace ex=ed::example;
namespace {
void Check(bool value,const std::source_location at=std::source_location::current()) {
    if (!value) throw std::runtime_error("Scene drag invariant at line "+std::to_string(at.line()));
}
bool Near(double a,double b) { return std::abs(a-b)<1e-4; }
}
void DragCases() {
    ex::Schema schema;
    ed::EditorWorkspace workspace{schema.View(),[&] { return ex::Options(schema); }};
    const auto scene=runtime::AssetId::FromString("example.scene"); const runtime::SceneNodeId node{42};
    const std::filesystem::path file{EPIDEMIC_EDITOR_TEST_DIRECTORY "/scene-drag.epidemic"};
    ed::SceneView view; view.Resize(600,400); ed::SceneDrag drag;
    const auto reset=[&] {
        drag.Cancel(); ex::Take(workspace.New(file,ex::Definition(schema),true)); ex::Take(workspace.Save());
        Check(workspace.Select({scene,ed::UiElement::SceneNode,node.Raw()})); view.Clear(); ex::Take(view.Update(*workspace.Project()->Snapshot(),scene));
    };
    const auto begin=[&](ed::GizmoTool tool,ed::GizmoHandle handle,ed::ViewPoint start,ed::GizmoSpace space=ed::GizmoSpace::Local) {
        ex::Take(drag.Begin(workspace,view,tool,space,handle,start));
    };
    const auto local=[&] { return std::get<game::composition::SceneCompositionDefinition>(workspace.Project()->Snapshot()->Find(scene)->data).scene.nodes[0].local_transform; };
    reset();
    const auto saved=workspace.Project()->Snapshot(); const auto revision=workspace.Project()->Revision();
    const ed::ViewPoint start{300,200};
    begin(ed::GizmoTool::Move,ed::GizmoHandle::Free,start);
    for (int i=1;i<=30;++i) ex::Take(drag.Move(view,{start.x+i*2,start.y-i}));
    Check(workspace.Project()->Snapshot()==saved && workspace.Project()->Revision()==revision && !workspace.Project()->CanUndo() && !workspace.Project()->IsDirty());
    Check(Near(view.Items()[0].world.position.x,60/view.Scale()));
    Check(ex::Take(drag.Commit(workspace))); Check(workspace.Project()->Revision()==revision+1);
    const auto moved=workspace.Project()->Snapshot();
    ex::Take(workspace.Undo()); Check(workspace.Project()->Snapshot()==saved && !workspace.Project()->CanUndo() && !workspace.Project()->IsDirty());
    ex::Take(workspace.Redo()); Check(workspace.Project()->Snapshot()==moved);
    reset(); begin(ed::GizmoTool::Move,ed::GizmoHandle::Free,start);
    ex::Take(drag.Move(view,{301,201})); Check(!ex::Take(drag.Commit(workspace)) && !workspace.Project()->IsDirty());
    begin(ed::GizmoTool::Move,ed::GizmoHandle::Free,start); ex::Take(drag.Move(view,{400,230}));
    drag.Cancel(); ex::Take(view.Update(*workspace.Project()->Snapshot(),scene));
    Check(local()==runtime::Transform{} && !workspace.Project()->CanUndo());
    begin(ed::GizmoTool::Move,ed::GizmoHandle::Free,start); ex::Take(drag.Move(view,{360,200}));
    Check(!drag.Move(view,{std::numeric_limits<double>::infinity(),0})); Check(!drag.Commit(workspace) && !workspace.Project()->IsDirty());
    reset(); begin(ed::GizmoTool::Move,ed::GizmoHandle::X,start,ed::GizmoSpace::World);
    ex::Take(drag.Move(view,{start.x+view.Scale()*2.3,start.y-100},1)); Check(ex::Take(drag.Commit(workspace)));
    Check(local().position.x==2 && local().position.y==0);
    reset(); ex::Take(drag.Begin(workspace,view,ed::GizmoTool::Move,ed::GizmoSpace::World,ed::GizmoHandle::Free,start,0));
    ex::Take(drag.Move(view,{start.x+.25,start.y})); Check(ex::Take(drag.Commit(workspace)) && local().position.x>0);
    // Revision protection includes Undo returning to exactly the same snapshot.
    reset(); const auto source=workspace.Project()->Snapshot();
    begin(ed::GizmoTool::Move,ed::GizmoHandle::Free,start); ex::Take(drag.Move(view,{360,200}));
    ex::Take(workspace.Execute({ed::RenameDocument{scene,"Changed"}},workspace.Project()->Revision())); ex::Take(workspace.Undo());
    Check(workspace.Project()->Snapshot()==source && !drag.Current(workspace)); Check(!drag.Commit(workspace) && workspace.Project()->CanRedo());
    reset(); const auto frame=ed::MakeGizmo(view,view.Items()[0].world,ed::GizmoSpace::Local);
    Check(ed::PickGizmo(frame,ed::GizmoTool::Move,frame.x)==ed::GizmoHandle::X);
    Check(ed::PickGizmo(frame,ed::GizmoTool::Scale,frame.pivot)==ed::GizmoHandle::Uniform);
    Check(ed::PickGizmo(frame,ed::GizmoTool::Rotate,frame.x)==ed::GizmoHandle::Ring);
    begin(ed::GizmoTool::Rotate,ed::GizmoHandle::Ring,frame.x);
    ex::Take(drag.Move(view,frame.y)); Check(ex::Take(drag.Commit(workspace)));
    Check(Near(local().rotation.z,std::sqrt(.5)) && Near(local().rotation.w,std::sqrt(.5)) && local().position==runtime::Vec3{});
    const auto rotated=ed::MakeGizmo(view,view.Items()[0].world,ed::GizmoSpace::Local);
    begin(ed::GizmoTool::Move,ed::GizmoHandle::X,rotated.x);
    ex::Take(drag.Move(view,{rotated.x.x,rotated.x.y-52})); Check(ex::Take(drag.Commit(workspace)));
    Check(Near(local().position.x,0) && local().position.y>0);
    reset(); begin(ed::GizmoTool::Scale,ed::GizmoHandle::X,frame.x);
    ex::Take(drag.Move(view,{frame.x.x+52,frame.x.y})); Check(ex::Take(drag.Commit(workspace)));
    Check(local().scale==runtime::Vec3{2,1,1});
    reset(); begin(ed::GizmoTool::Scale,ed::GizmoHandle::Uniform,frame.pivot);
    ex::Take(drag.Move(view,{frame.pivot.x+52,frame.pivot.y-52})); Check(ex::Take(drag.Commit(workspace)));
    Check(local().scale==runtime::Vec3{2,2,1});
    reset(); begin(ed::GizmoTool::Scale,ed::GizmoHandle::X,frame.x);
    Check(!drag.Move(view,frame.pivot)); Check(!drag.Commit(workspace) && !workspace.Project()->CanUndo());
    begin(ed::GizmoTool::Scale,ed::GizmoHandle::X,frame.x);
    ex::Take(drag.Move(view,{frame.pivot.x-52,frame.pivot.y})); Check(ex::Take(drag.Commit(workspace)) && local().scale.x==-1);
    // Parented translation is delegated to existing Runtime world/local conversion.
    reset(); runtime::SceneNodeSnapshot parent; parent.id=runtime::SceneNodeId{43};
    parent.local_transform.position={10,20,1e20f}; parent.local_transform.scale={2,3,2}; parent.local_transform.rotation={0,0,std::sqrt(.5f),std::sqrt(.5f)};
    auto authored=runtime::Transform{}; authored.position.z=3; authored.scale={.3f,.7f,1.3f};
    ex::Take(workspace.Execute({ed::AddSceneNode{scene,parent},ed::SetSceneTransform{scene,node,authored},ed::SetSceneParent{scene,node,parent.id}},workspace.Project()->Revision()));
    ex::Take(view.Update(*workspace.Project()->Snapshot(),scene)); const auto before=view.Items()[0].world;
    begin(ed::GizmoTool::Move,ed::GizmoHandle::Free,start,ed::GizmoSpace::World);
    ex::Take(drag.Move(view,{start.x+2*view.Scale(),start.y-3*view.Scale()}));
    Check(Near(view.Items()[0].world.position.x,before.position.x+2) && Near(view.Items()[0].world.position.y,before.position.y+3));
    Check(ex::Take(drag.Commit(workspace)));
    Check(local().position.z==authored.position.z && local().rotation==authored.rotation && local().scale==authored.scale);
    ex::Take(workspace.Save());
    ex::Take(workspace.Open(file)); Check(workspace.Select({scene,ed::UiElement::SceneNode,node.Raw()}));
    ex::Take(workspace.Play()); const auto frozen=workspace.Session()->Scene()->scene.snapshots->CaptureSnapshot();
    ex::Take(view.Update(*workspace.Project()->Snapshot(),scene));
    begin(ed::GizmoTool::Move,ed::GizmoHandle::Free,start); ex::Take(drag.Move(view,{start.x+52,start.y})); Check(ex::Take(drag.Commit(workspace)));
    const auto still=workspace.Session()->Scene()->scene.snapshots->CaptureSnapshot();
    Check(frozen.nodes[0].local_transform==still.nodes[0].local_transform); ex::Take(workspace.Stop());
    ex::Take(workspace.Play()); ex::Take(workspace.Stop());
    reset(); auto tilted=runtime::Transform{}; tilted.rotation={std::sqrt(.5f),0,0,std::sqrt(.5f)};
    ex::Take(workspace.Execute({ed::SetSceneTransform{scene,node,tilted}},workspace.Project()->Revision()));
    Check(!ed::SceneDrag::Supports(*workspace.Project()->Snapshot(),workspace.Selection()));
    Check(!drag.Begin(workspace,view,ed::GizmoTool::Move,ed::GizmoSpace::World,ed::GizmoHandle::Free,start));
}
