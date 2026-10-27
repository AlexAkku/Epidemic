#include "Epidemic/Game/Editor/editor_workspace.h"
#include <algorithm>
#include <charconv>
#include <cmath>
#include <limits>

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
    project_ = std::move(candidate).Value(); session_.reset(); selection_ = {};
    return Finish(foundation::Result<void>::Success(), true);
}
foundation::Result<void> EditorWorkspace::Open(const std::filesystem::path &file, bool discard_dirty) {
    if (project_ && project_->IsDirty() && !discard_dirty) return Finish(foundation::Result<void>::Failure(Error("editor.ui.unsaved_project")));
    return Adopt(EditorProject::Open(file, schema_));
}
foundation::Result<void> EditorWorkspace::New(const std::filesystem::path &file, ProjectDefinition definition, bool discard_dirty) {
    if (project_ && project_->IsDirty() && !discard_dirty) return Finish(foundation::Result<void>::Failure(Error("editor.ui.unsaved_project")));
    return Adopt(EditorProject::Create(file, std::move(definition), schema_));
}
foundation::Result<void> EditorWorkspace::Execute(std::vector<Command> commands, std::uint64_t expected_revision) {
    if (!project_) return Finish(foundation::Result<void>::Failure(Error("editor.ui.no_project")));
    const auto changed = project_->Execute({expected_revision, std::move(commands)});
    return Finish(changed ? foundation::Result<void>::Success() : foundation::Result<void>::Failure(changed.GetError()), true);
}
foundation::Result<void> EditorWorkspace::Save() {
    return Finish(project_ ? project_->Save() : foundation::Result<void>::Failure(Error("editor.ui.no_project")));
}
foundation::Result<void> EditorWorkspace::Undo() {
    return Finish(project_ ? project_->Undo() : foundation::Result<void>::Failure(Error("editor.ui.no_project")), true);
}
foundation::Result<void> EditorWorkspace::Redo() {
    return Finish(project_ ? project_->Redo() : foundation::Result<void>::Failure(Error("editor.ui.no_project")), true);
}
foundation::Result<void> EditorWorkspace::Play() {
    if (!project_) return Finish(foundation::Result<void>::Failure(Error("editor.ui.no_project")));
    if (session_ && session_->IsPlaying()) return Finish(foundation::Result<void>::Failure(Error("editor.ui.already_playing")));
    auto candidate = project_->Play(play_options_());
    if (!candidate) return Finish(foundation::Result<void>::Failure(candidate.GetError()));
    session_ = std::move(candidate).Value(); return Finish(foundation::Result<void>::Success());
}
foundation::Result<void> EditorWorkspace::Stop() {
    return Finish(session_ ? session_->Stop() : foundation::Result<void>::Success());
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
    if (!project_) return false;
    const auto *doc = project_->Snapshot()->Find(selection.document); if (!doc) return false;
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
    if (!project_ || selection_.kind != UiElement::SceneNode) return std::nullopt;
    const auto snapshot = project_->Snapshot();
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
    if (project_ && project_->Snapshot()->Find(selection_.document)) { selection_ = {selection_.document}; return; }
    if (project_ && !project_->Snapshot()->definition.documents.empty()) selection_ = {project_->Snapshot()->definition.documents.front().id};
    else selection_ = {};
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
