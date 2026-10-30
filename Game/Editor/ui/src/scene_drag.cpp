#include "Epidemic/Game/Editor/scene_drag.h"
#include <algorithm>
#include <cmath>
#include <limits>
#include <numbers>
#include <unordered_map>

namespace epidemic::game::editor {
namespace {
bool Finite(ViewPoint p) {
    return std::isfinite(p.x) && std::isfinite(p.y);
}

double Dot(ViewPoint a, ViewPoint b) {
    return a.x * b.x + a.y * b.y;
}

ViewPoint Difference(ViewPoint a, ViewPoint b) {
    return {a.x - b.x, a.y - b.y};

}

double Length(ViewPoint p) {
    return std::hypot(p.x, p.y);
}

foundation::Result<void> Failure(std::string_view code, std::string_view message) {
    return foundation::Result<void>::Failure(foundation::Error::Create(code, message));
}

bool Store(double value, float &output) {
    if (!std::isfinite(value) || std::abs(value) > std::numeric_limits<float>::max())
        return false;
    const auto converted = static_cast<float>(value);
    if (value != 0 && converted == 0)
        return false;
    output = converted;
    return true;
}

double Snap(double value, double step) {
    return step > 0 ? std::round(value / step) * step : value;
}

} // namespace

GizmoFrame MakeGizmo(const SceneView &view, const runtime::Transform &world, GizmoSpace space) {
    const auto x = space == GizmoSpace::Local ? runtime::RotateVector(world.rotation, {1, 0, 0})
                                              : runtime::Vec3{1, 0, 0};

    const auto y = space == GizmoSpace::Local ? runtime::RotateVector(world.rotation, {0, 1, 0})
                                              : runtime::Vec3{0, 1, 0};

    const auto p = view.ToScreen({world.position.x, world.position.y});
    return {p, {p.x + x.x * 52, p.y - x.y * 52}, {p.x + y.x * 52, p.y - y.y * 52}};

}

GizmoHandle PickGizmo(const GizmoFrame &frame, GizmoTool tool, ViewPoint cursor) {
    if (!Finite(cursor) || !Finite(frame.pivot))
        return GizmoHandle::None;
    const auto delta = Difference(cursor, frame.pivot);
    if (tool == GizmoTool::Rotate)
        return std::abs(Length(delta) - 52) <= 7 ? GizmoHandle::Ring : GizmoHandle::None;
    if (std::abs(delta.x) <= 8 && std::abs(delta.y) <= 8)
        return tool == GizmoTool::Scale ? GizmoHandle::Uniform : GizmoHandle::Free;
    const auto axis = [&](ViewPoint end) {
        const auto direction = Difference(end, frame.pivot);
        const auto along = Dot(delta, direction) / Dot(direction, direction);
        return along >= .20 && along <= 1.16 &&
               Length({delta.x - along * direction.x, delta.y - along * direction.y}) <= 7;
    };

    if (axis(frame.x))
        return GizmoHandle::X;
    if (axis(frame.y))
        return GizmoHandle::Y;
    return GizmoHandle::None;
}

bool SceneDrag::Supports(const ProjectSnapshot &snapshot, const UiSelection &selection) {
    const auto *doc = snapshot.Find(selection.document);
    const auto *definition =
        doc ? std::get_if<composition::SceneCompositionDefinition>(&doc->data) : nullptr;
    if (!definition || selection.kind != UiElement::SceneNode || selection.element == 0)
        return false;
    std::unordered_map<runtime::SceneNodeId, const runtime::SceneNodeSnapshot *> nodes;
    for (const auto &node : definition->scene.nodes)
        nodes.emplace(node.id, &node);
    auto id = runtime::SceneNodeId{selection.element};

    for (std::size_t depth = 0; id.IsValid() && depth <= nodes.size(); ++depth) {
        const auto found = nodes.find(id);
        if (found == nodes.end())
            return false;
        const auto &node = *found->second;
        if (node.local_transform.rotation.x != 0 || node.local_transform.rotation.y != 0)
            return false;
        id = node.parent;
    }

    return !id.IsValid();
}

foundation::Result<void> SceneDrag::Begin(const EditorWorkspace &workspace, const SceneView &view,
                                          GizmoTool tool, GizmoSpace space, GizmoHandle handle,
                                          ViewPoint cursor, double threshold) {
    Cancel();
    const auto *project = workspace.Project();
    if (!project || !Finite(cursor) || !std::isfinite(threshold) || threshold < 0 ||
        handle == GizmoHandle::None || !Supports(*project->Snapshot(), workspace.Selection()))
        return Failure("editor.drag_target",
                       "Select an XY Scene node with an XY parent hierarchy.");
    if ((tool == GizmoTool::Move && handle != GizmoHandle::Free && handle != GizmoHandle::X &&
         handle != GizmoHandle::Y) ||
        (tool == GizmoTool::Rotate && handle != GizmoHandle::Ring) ||
        (tool == GizmoTool::Scale &&
         (space != GizmoSpace::Local || (handle != GizmoHandle::X && handle != GizmoHandle::Y &&
                                         handle != GizmoHandle::Uniform))))
        return Failure("editor.drag_mode", "This handle/coordinate mode is not supported.");
    source_ = project->Snapshot();
    selection_ = workspace.Selection();
    revision_ = project->Revision();
    const auto &definition =
        std::get<composition::SceneCompositionDefinition>(source_->Find(selection_.document)->data);
    auto instance = composition::CreateSceneComposition(definition);
    if (!instance) {
        Cancel();
        return foundation::Result<void>::Failure(instance.GetError());
    }

    auto owned =
        std::make_unique<composition::SceneCompositionInstance>(std::move(instance).Value());
    runtime_node_ = owned->nodes.at(runtime::SceneNodeId{selection_.element});
    const auto local = owned->scene.transforms->GetLocalTransform(runtime_node_),
               world = owned->scene.transforms->GetWorldTransform(runtime_node_);
    if (!local || !world) {
        Cancel();
        return Failure("editor.drag_transform", "Scene transform is unavailable.");
    }

    original_ = *local;
    candidate_ = *local;
    world_ = *world;
    const auto frame = MakeGizmo(view, world_, space);
    pivot_ = frame.pivot;
    x_ = {(frame.x.x - pivot_.x) / 52, -(frame.x.y - pivot_.y) / 52};

    y_ = {(frame.y.x - pivot_.x) / 52, -(frame.y.y - pivot_.y) / 52};

    start_ = cursor;
    scale_ = view.Scale();
    threshold_ = threshold;
    tool_ = tool;
    space_ = space;
    handle_ = handle;
    scene_ = std::move(owned);
    return foundation::Result<void>::Success();
}

foundation::Result<void> SceneDrag::Move(SceneView &view, ViewPoint cursor, double snap) {
    if (!Active())
        return Failure("editor.drag_inactive", "No active gesture.");
    valid_ = false;
    if (!Finite(cursor) || !std::isfinite(snap) || snap < 0)
        return Failure("editor.drag_value",
                       "Gesture values must be finite; snap must be nonnegative.");
    const auto pixels = Difference(cursor, start_);
    moved_ = moved_ || Length(pixels) >= threshold_;
    valid_ = true;
    if (!moved_)
        return foundation::Result<void>::Success();
    valid_ = false;
    auto next = original_;
    if (tool_ == GizmoTool::Move) {
        const ViewPoint delta{pixels.x / scale_, -pixels.y / scale_};

        auto desired = world_;
        if (handle_ == GizmoHandle::Free) {
            if (!Store(Snap(world_.position.x + delta.x, snap), desired.position.x) ||
                !Store(Snap(world_.position.y + delta.y, snap), desired.position.y))
                return Failure("editor.drag_range", "Position is outside the finite float range.");
        } else {
            const auto basis = handle_ == GizmoHandle::X ? x_ : y_;
            const double origin = handle_ == GizmoHandle::X ? world_.position.x : world_.position.y;
            const double distance = space_ == GizmoSpace::World
                                        ? Snap(origin + Dot(delta, basis), snap) - origin
                                        : Snap(Dot(delta, basis), snap);
            if (!Store(world_.position.x + basis.x * distance, desired.position.x) ||
                !Store(world_.position.y + basis.y * distance, desired.position.y))
                return Failure("editor.drag_range", "Position is outside the finite float range.");
        }

        // Avoid a world/local round-trip when the gesture returns exactly home.
        if (desired == world_) {
            const auto restored =
                scene_->scene.transforms->SetLocalTransform(runtime_node_, original_);
            if (!restored)
                return restored;
        } else {
            const auto applied =
                scene_->scene.transforms->SetWorldTransform(runtime_node_, desired);
            if (!applied)
                return applied;
        }

        const auto local = scene_->scene.transforms->GetLocalTransform(runtime_node_);
        if (!local)
            return Failure("editor.drag_transform", "Local transform is unavailable.");
        // World/local conversion may round untouched components. XY movement
        // must preserve the original authored Z, rotation and scale exactly.
        next.position.x = local->position.x;
        next.position.y = local->position.y;
    } else if (tool_ == GizmoTool::Rotate) {
        const auto a = Difference(start_, pivot_), b = Difference(cursor, pivot_);
        if (Length(a) < 8 || Length(b) < 8)
            return Failure("editor.drag_pivot", "Rotate using the ring, away from the pivot.");
        const double angle =
            std::remainder(std::atan2(-b.y, b.x) - std::atan2(-a.y, a.x), 2 * std::numbers::pi);
        if (angle != 0)
            next.rotation = runtime::Multiply(original_.rotation,
                                              {0, 0, static_cast<float>(std::sin(angle * .5)),
                                               static_cast<float>(std::cos(angle * .5))});
    } else {
        const ViewPoint delta{pixels.x, -pixels.y};

        const double factor = handle_ == GizmoHandle::Uniform
                                  ? 1 + (delta.x + delta.y) / 104
                                  : 1 + Dot(delta, handle_ == GizmoHandle::X ? x_ : y_) / 52;
        if ((handle_ != GizmoHandle::Y && !Store(original_.scale.x * factor, next.scale.x)) ||
            (handle_ != GizmoHandle::X && !Store(original_.scale.y * factor, next.scale.y)))
            return Failure("editor.drag_range", "Scale is outside the finite float range.");
    }

    if (!runtime::IsValidTransform(next))
        return Failure("editor.drag_value",
                       "Transform must be finite and scale must remain nonzero.");
    const auto applied = scene_->scene.transforms->SetLocalTransform(runtime_node_, next);
    if (!applied)
        return applied;
    if (!view.Preview(*scene_))
        return Failure("editor.drag_range", "Preview bounds are not finite.");
    candidate_ = next;
    valid_ = true;
    return foundation::Result<void>::Success();
}

bool SceneDrag::Current(const EditorWorkspace &workspace) const {
    const auto *project = workspace.Project();
    return Active() && project && project->Snapshot() == source_ &&
           project->Revision() == revision_ && workspace.Selection() == selection_;
}

foundation::Result<bool> SceneDrag::Commit(EditorWorkspace &workspace) {
    using Result = foundation::Result<bool>;
    if (!Current(workspace) || !valid_) {
        Cancel();
        return Result::Failure(
            foundation::Error::
                Create("editor.drag_rejected",
                       "Gesture is invalid or its source revision/selection changed."));
    }

    if (!moved_ || candidate_ == original_) {
        Cancel();
        return Result::Success(false);
    }

    const Command command = SetSceneTransform{selection_.document,
                                              runtime::SceneNodeId{selection_.element}, candidate_};

    const auto revision = revision_;
    Cancel();
    const auto committed = workspace.Execute({command}, revision);
    if (!committed)
        return Result::Failure(committed.GetError());
    return Result::Success(true);
}

void SceneDrag::Cancel() noexcept {
    scene_.reset();
    source_.reset();
    moved_ = false;
    valid_ = true;
}

} // namespace epidemic::game::editor
