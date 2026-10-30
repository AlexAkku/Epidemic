#include "Epidemic/Game/Editor/editor_workspace.h"
#include "Epidemic/Game/Editor/image_resource.h"
#include <algorithm>
#include <charconv>
#include <cmath>
#include <limits>
#include <numbers>

namespace epidemic::game::editor {
namespace {
foundation::Error Error(std::string_view code) { return foundation::Error::Create(code, "editor UI action rejected"); }
const comp::PrefabNode *FindNode(const comp::PrefabNode &node, std::uint64_t id) {
    if (node.id.Raw() == id) return &node;
    for (const auto &child : node.children) if (const auto *found = FindNode(child, id)) return found;
    return nullptr;
}
}
foundation::Result<void> EditorWorkspace::Finish(foundation::Result<void> &&result, bool reconcile) {
    if (!result) error_ = result.GetError();
    else { error_.reset(); if (reconcile) Reconcile(); }
    return std::move(result);
}
foundation::Result<void> EditorWorkspace::Adopt(foundation::Result<std::unique_ptr<EditorProject>> candidate) {
    if (!candidate) return Finish(foundation::Result<void>::Failure(candidate.GetError()));
    const auto stopped = Stop(); if (!stopped) return stopped;
    hot_reload_.Cancel();hot_reload_.SetEnabled(false);folder_.reset(); project_ = std::move(candidate).Value(); session_.reset(); selection_ = {};
    return Finish(foundation::Result<void>::Success(), true);
}
foundation::Result<void> EditorWorkspace::Open(const std::filesystem::path &file, bool discard_dirty) {
    if (IsDirty() && !discard_dirty) return Finish(foundation::Result<void>::Failure(Error("editor.ui.unsaved_project")));
    if (file.extension() == ".epidemicproj") return OpenFolder(file.parent_path(), discard_dirty);
    return Adopt(EditorProject::Open(file, schema_));
}
foundation::Result<void> EditorWorkspace::New(const std::filesystem::path &file, ProjectDefinition definition, bool discard_dirty) {
    if (IsDirty() && !discard_dirty) return Finish(foundation::Result<void>::Failure(Error("editor.ui.unsaved_project")));
    return Adopt(EditorProject::Create(file, std::move(definition), schema_));
}
foundation::Result<void> EditorWorkspace::OpenFolder(const std::filesystem::path &root, bool discard_dirty) {
    if (IsDirty() && !discard_dirty) return Finish(foundation::Result<void>::Failure(Error("editor.ui.unsaved_project")));
    auto candidate = ProjectFolder::Open(root, schema_);
    if (!candidate) return Finish(foundation::Result<void>::Failure(candidate.GetError()));
    const auto stopped = Stop(); if (!stopped) return stopped;
    project_.reset(); folder_ = std::move(candidate).Value(); hot_reload_.Reset(*folder_);hot_reload_.SetEnabled(content_watch_enabled_); session_.reset(); selection_ = {};
    return Finish(foundation::Result<void>::Success(), true);
}
foundation::Result<void> EditorWorkspace::NewFolder(const std::filesystem::path &root, std::string title, ProjectDefinition definition, bool discard_dirty) {
    if (IsDirty() && !discard_dirty) return Finish(foundation::Result<void>::Failure(Error("editor.ui.unsaved_project")));
    auto candidate = ProjectFolder::Create(root, std::move(title), std::move(definition), schema_);
    if (!candidate) return Finish(foundation::Result<void>::Failure(candidate.GetError()));
    const auto stopped = Stop(); if (!stopped) return stopped;
    project_.reset(); folder_ = std::move(candidate).Value(); hot_reload_.Reset(*folder_);hot_reload_.SetEnabled(content_watch_enabled_); session_.reset(); selection_ = {};
    return Finish(foundation::Result<void>::Success(), true);
}
foundation::Result<void> EditorWorkspace::CloneFolder(const std::filesystem::path &destination) {
    return Finish(folder_ ? folder_->CloneTo(destination) : foundation::Result<void>::Failure(Error("editor.ui.no_folder")));
}
foundation::Result<void> EditorWorkspace::ImportPng(const std::filesystem::path &source, std::string_view relative) {
    if (!folder_) return Finish(foundation::Result<void>::Failure(Error("editor.ui.no_folder")));
    const auto imported=ImportExternalPng(*folder_,source,relative);
    return Finish(imported?foundation::Result<void>::Success():foundation::Result<void>::Failure(imported.GetError()));
}
foundation::Result<void> EditorWorkspace::ReimportPng(runtime::AssetId asset) {
    if (!folder_) return Finish(foundation::Result<void>::Failure(Error("editor.ui.no_folder")));
    for(const auto &record:folder_->Sources())if(record.id==asset){
        if(record.type!=runtime::AssetType{foundation::StringId::FromString("epidemic.texture.png")})
            return Finish(foundation::Result<void>::Failure(Error("editor.image_type")));
        return Finish(hot_reload_.Request(*folder_,asset)); // Completion/error arrives through PollContent.
    }
    return Finish(foundation::Result<void>::Failure(Error("editor.ui.unknown_asset")));
}
foundation::Result<void> EditorWorkspace::AssignPrefabSprite(runtime::AssetId prefab, runtime::AssetId texture) {
    return Finish(folder_?folder_->SetPrefabSprite(prefab,texture):foundation::Result<void>::Failure(Error("editor.ui.no_folder")));
}
foundation::Result<void> EditorWorkspace::MoveAsset(runtime::AssetId asset,std::string_view relative) {
    return Finish(folder_?folder_->MoveSource(asset,relative):foundation::Result<void>::Failure(Error("editor.ui.no_folder")));
}
foundation::Result<void> EditorWorkspace::UndoAssetMove() {
    return Finish(folder_?folder_->UndoAssetMove():foundation::Result<void>::Failure(Error("editor.ui.no_folder")));
}
foundation::Result<void> EditorWorkspace::RedoAssetMove() {
    return Finish(folder_?folder_->RedoAssetMove():foundation::Result<void>::Failure(Error("editor.ui.no_folder")));
}
foundation::Result<void> EditorWorkspace::Execute(std::vector<Command> commands, std::uint64_t expected_revision) {
    if (!Project()) return Finish(foundation::Result<void>::Failure(Error("editor.ui.no_project")));
    const EditRequest request{expected_revision, std::move(commands)};
    const auto changed = folder_ ? folder_->Execute(request) : project_->Execute(request);
    return Finish(changed ? foundation::Result<void>::Success() : foundation::Result<void>::Failure(changed.GetError()), true);
}
foundation::Result<void> EditorWorkspace::Save() {
    return Finish(folder_ ? folder_->Save() : project_ ? project_->Save() : foundation::Result<void>::Failure(Error("editor.ui.no_project")));
}
foundation::Result<void> EditorWorkspace::Undo() {
    return Finish(folder_ ? folder_->Undo() : project_ ? project_->Undo() : foundation::Result<void>::Failure(Error("editor.ui.no_project")), true);
}
foundation::Result<void> EditorWorkspace::Redo() {
    return Finish(folder_ ? folder_->Redo() : project_ ? project_->Redo() : foundation::Result<void>::Failure(Error("editor.ui.no_project")), true);
}
foundation::Result<void> EditorWorkspace::Play() {
    if (!Project()) return Finish(foundation::Result<void>::Failure(Error("editor.ui.no_project")));
    if (session_ && session_->IsPlaying()) return Finish(foundation::Result<void>::Failure(Error("editor.ui.already_playing")));
    auto candidate = folder_ ? folder_->Play(play_options_()) : project_->Play(play_options_());
    if (!candidate) return Finish(foundation::Result<void>::Failure(candidate.GetError()));
    session_ = std::move(candidate).Value(); return Finish(foundation::Result<void>::Success());
}
foundation::Result<void> EditorWorkspace::Stop() {
    auto stopped=session_ ? session_->Stop() : foundation::Result<void>::Success();
    return Finish(std::move(stopped));
}
foundation::Result<void> EditorWorkspace::Tick(runtime::GameDuration advance) {
    if (!session_ || !session_->IsPlaying()) return foundation::Result<void>::Success();
    auto result = session_->Tick(advance); if (!result) error_ = result.GetError(); return result;
}
foundation::Result<std::size_t> EditorWorkspace::Send(runtime::SceneNodeId placement, const comp::GameEvent &event) {
    auto result = session_ ? session_->Send(placement, event) : foundation::Result<std::size_t>::Failure(Error("editor.ui.not_playing"));
    if (!result) error_ = result.GetError(); else error_.reset(); return result;
}
std::vector<std::shared_ptr<const PlayActivation>> EditorWorkspace::Activations() const { return session_ ? session_->Activations() : std::vector<std::shared_ptr<const PlayActivation>>{}; }
bool EditorWorkspace::Exists(const UiSelection &selection) const {
    if (!Project()) return false;
    const auto *doc = Project()->Snapshot()->Find(selection.document); if (!doc) return false;
    if (selection.kind != UiElement::PrefabComponent && (selection.component.IsValid() || selection.part.IsValid())) return false;
    if (selection.kind == UiElement::Document) return selection.element == 0;
    if (const auto *scene = std::get_if<composition::SceneCompositionDefinition>(&doc->data))
        return selection.kind == UiElement::SceneNode && std::any_of(scene->scene.nodes.begin(), scene->scene.nodes.end(), [&](const auto &n) { return n.id.Raw() == selection.element; });
    if (const auto *prefab = std::get_if<comp::PrefabDefinition>(&doc->data)) {
        const auto *node = FindNode(prefab->root, selection.element);
        if (!node) return false;
        if (selection.kind == UiElement::PrefabNode) return true;
        return selection.kind == UiElement::PrefabComponent && std::any_of(node->components.begin(), node->components.end(),
            [&](const auto &component) { return component.type == selection.component && component.part == selection.part; });
    }
    if (const auto *graph = std::get_if<runtime::graph::GraphDefinition>(&doc->data)) {
        if (selection.kind == UiElement::GraphNode) return std::any_of(graph->nodes.begin(), graph->nodes.end(), [&](const auto &n) { return n.id.Raw() == selection.element; });
        if (selection.kind == UiElement::Connection) return std::any_of(graph->connections.begin(), graph->connections.end(), [&](const auto &c) { return c.id.Raw() == selection.element; });
    }
    return false;
}
bool EditorWorkspace::Select(const UiSelection &selection) { if (!Exists(selection)) return false; selection_ = selection; return true; }
std::optional<UiSelection> EditorWorkspace::PrefabDefinitionSelection() const {
    if (!Project() || selection_.kind != UiElement::SceneNode) return std::nullopt;
    const auto snapshot = Project()->Snapshot();
    const auto *document = snapshot->Find(selection_.document);
    const auto *scene = document ? std::get_if<composition::SceneCompositionDefinition>(&document->data) : nullptr;
    if (!scene) return std::nullopt;
    for (const auto &placement : scene->placements) if (placement.node.Raw() == selection_.element) {
        const auto *target = snapshot->Find(placement.prefab);
        const auto *prefab = target ? std::get_if<comp::PrefabDefinition>(&target->data) : nullptr;
        if (prefab) return UiSelection{target->id, UiElement::PrefabNode, prefab->root.id.Raw()};
    }
    return std::nullopt;
}
void EditorWorkspace::Reconcile() {
    if (Exists(selection_)) return;
    if (selection_.kind == UiElement::PrefabComponent) {
        const UiSelection owner{selection_.document, UiElement::PrefabNode, selection_.element};
        if (Exists(owner)) { selection_ = owner; return; }
    }
    if (Project() && Project()->Snapshot()->Find(selection_.document)) { selection_ = {selection_.document}; return; }
    if (Project() && !Project()->Snapshot()->definition.documents.empty()) selection_ = {Project()->Snapshot()->definition.documents.front().id};
    else selection_ = {};
}
foundation::Result<Command> EditorWorkspace::TransformCommand(runtime::AssetId document, runtime::SceneNodeId node,
    const runtime::Transform &current, UiTransformField field, std::string_view text) {
    using Result = foundation::Result<Command>;
    const auto reject = [](std::string_view code, std::string_view message) { return Result::Failure(foundation::Error::Create(code, message)); };
    const auto parsed = ParseValue(comp::PropertyKind::Real, text);
    if (!parsed) return reject("editor.ui.transform_value", "Enter a finite number for the local transform.");
    const double real = std::get<double>(parsed.Value());
    auto transform = current;
    if (field == UiTransformField::RotationZDegrees) {
        if (current.rotation.x != 0 || current.rotation.y != 0)
            return reject("editor.ui.rotation_profile", "Z rotation editing requires an XY-plane orientation; the existing quaternion is preserved.");
        const double half_angle = std::remainder(real, 360.0) * std::numbers::pi / 360.0;
        transform.rotation = runtime::Normalize(runtime::Quat{0, 0, static_cast<float>(std::sin(half_angle)), static_cast<float>(std::cos(half_angle))});
    } else {
        if (std::abs(real) > std::numeric_limits<float>::max()) return reject("editor.ui.transform_range", "Value exceeds the Scene float range.");
        const float value = static_cast<float>(real);
        if (real != 0 && value == 0) return reject("editor.ui.transform_range", "Value is too small for the Scene float representation.");
        switch (field) {
        case UiTransformField::PositionX: transform.position.x = value; break;
        case UiTransformField::PositionY: transform.position.y = value; break;
        case UiTransformField::PositionZ: transform.position.z = value; break;
        case UiTransformField::ScaleX: transform.scale.x = value; break;
        case UiTransformField::ScaleY: transform.scale.y = value; break;
        case UiTransformField::ScaleZ: transform.scale.z = value; break;
        default: return reject("editor.ui.transform_field", "Unknown local transform field.");
        }
    }
    if (!runtime::IsValidTransform(transform)) return reject("editor.ui.invalid_transform", "Scene requires a normalized rotation and finite, non-zero scale.");
    return Result::Success(SetSceneTransform{document, node, transform});
}
foundation::Result<comp::PropertyValue> EditorWorkspace::ParseValue(comp::PropertyKind kind, std::string_view text) {
    using Result = foundation::Result<comp::PropertyValue>;
    if (kind == comp::PropertyKind::String) return Result::Success(std::string{text});
    while (!text.empty() && (text.front() == ' ' || text.front() == '\t')) text.remove_prefix(1);
    while (!text.empty() && (text.back() == ' ' || text.back() == '\t')) text.remove_suffix(1);
    if (text.empty()) return Result::Failure(Error("editor.ui.invalid_value"));
    if (kind == comp::PropertyKind::Boolean) {
        if (text == "true") return Result::Success(true);
        if (text == "false") return Result::Success(false);
    } else if (kind == comp::PropertyKind::Integer) {
        std::int64_t value = 0; const auto parsed = std::from_chars(text.data(), text.data() + text.size(), value);
        if (parsed.ec == std::errc{} && parsed.ptr == text.data() + text.size()) return Result::Success(value);
    } else if (kind == comp::PropertyKind::Real) {
        double value = 0; const auto parsed = std::from_chars(text.data(), text.data() + text.size(), value);
        if (parsed.ec == std::errc{} && parsed.ptr == text.data() + text.size() && std::isfinite(value)) return Result::Success(value);
    }
    return Result::Failure(Error("editor.ui.invalid_value"));
}
std::string EditorWorkspace::FormatValue(const comp::PropertyValue &value) {
    return std::visit([](const auto &item) -> std::string {
        using T = std::decay_t<decltype(item)>;
        if constexpr (std::is_same_v<T, std::string>) return item;
        else if constexpr (std::is_same_v<T, bool>) return item ? "true" : "false";
        else {
            char buffer[128]; const auto result = std::to_chars(std::begin(buffer), std::end(buffer), item);
            return result.ec == std::errc{} ? std::string(buffer, result.ptr) : std::string{};
        }
    }, value);
}
} // namespace epidemic::game::editor
