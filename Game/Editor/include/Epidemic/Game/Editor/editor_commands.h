#pragma once
#include "Epidemic/Game/Editor/editor_project.h"

namespace epidemic::game::editor {
struct CreateDocument {
    EditorDocument document;
};

struct DeleteDocument {
    runtime::AssetId document;
};

struct DuplicateDocument {
    runtime::AssetId document;
    runtime::AssetId copy;
    std::string name;
};

// Restore only this document from the native durable save point. The caller
// cannot supply replacement data or resurrect arbitrary retired identities.
struct DiscardDocument {
    runtime::AssetId document;
};

struct RenameDocument {
    runtime::AssetId document;
    std::string name;
};

struct SetEntryScene {
    runtime::AssetId document;
};

struct EditSystem {
    runtime::AssetId document;
    ProjectSystem system;
    bool remove = false;
};

struct EditPrefabLink {
    runtime::AssetId document;
    PrefabLink link;
    bool remove = false;
};

struct AddEntity {
    runtime::AssetId document;
    comp::PrefabNodeId parent;
    comp::PrefabNode node;
};

struct DeleteEntity {
    runtime::AssetId document;
    comp::PrefabNodeId node;
};

struct ReparentPrefabNode {
    runtime::AssetId document;
    comp::PrefabNodeId node, parent;
};

struct SetComponent {
    runtime::AssetId document;
    comp::PrefabNodeId node;
    comp::PrefabComponent component;
};

struct DeleteComponent {
    runtime::AssetId document;
    comp::PrefabNodeId node;
    gameplay::TypeId type;
    gameplay::entities::EntityPartId part;
};

struct SetProperty {
    runtime::AssetId document;
    comp::PrefabNodeId node;
    gameplay::TypeId component;
    gameplay::entities::EntityPartId part;
    comp::PrefabProperty property;
};

struct SetBehavior {
    runtime::AssetId document;
    comp::PrefabNodeId node;
    gameplay::TypeId component;
    gameplay::entities::EntityPartId part;
    comp::GraphBehaviorDefinition behavior;
};

struct DeleteBehavior {
    runtime::AssetId document;
    comp::PrefabNodeId node;
    gameplay::TypeId component;
    gameplay::entities::EntityPartId part;
    foundation::StringId behavior;
};

struct AddGraphNode {
    runtime::AssetId document;
    runtime::graph::NodeDefinition node;
};

struct DeleteGraphNode {
    runtime::AssetId document;
    runtime::graph::ElementId node;
};

struct ConnectPorts {
    runtime::AssetId document;
    runtime::graph::ConnectionDefinition connection;
};

struct DisconnectPorts {
    runtime::AssetId document;
    runtime::graph::ElementId connection;
};

struct AddSceneNode {
    runtime::AssetId document;
    runtime::SceneNodeSnapshot node;
};

struct DeleteSceneNode {
    runtime::AssetId document;
    runtime::SceneNodeId node;
};

struct SetSceneParent {
    runtime::AssetId document;
    runtime::SceneNodeId node, parent;
};

struct RenameSceneNode {
    runtime::AssetId document;
    runtime::SceneNodeId node;
    std::string name;
};

struct OrderSceneNode {
    runtime::AssetId document;
    runtime::SceneNodeId node, before;
};

struct SetSceneTransform {
    runtime::AssetId document;
    runtime::SceneNodeId node;
    runtime::Transform transform;
};

struct PlacePrefab {
    runtime::AssetId document;
    composition::ScenePrefabPlacement placement;
};

struct SetPrefabParameter {
    runtime::AssetId document;
    comp::PrefabParameter parameter;
};

// Patch one override against the current placement; never overwrite unrelated
// assignments. The expected Prefab rejects commands held across reassignment.
struct SetComponentOverride {
    runtime::AssetId document;
    runtime::SceneNodeId placement;
    runtime::AssetId prefab;
    comp::ComponentOverride value;
    bool reset = false;
};

struct SetParameterOverride {
    runtime::AssetId document;
    runtime::SceneNodeId placement;
    runtime::AssetId prefab;
    comp::PropertyAssignment value;
    bool reset = false;
};

using Command =
    std::variant<CreateDocument, DeleteDocument, DuplicateDocument, DiscardDocument, RenameDocument, SetEntryScene, EditSystem, EditPrefabLink,
                 AddEntity, DeleteEntity, ReparentPrefabNode, SetComponent, DeleteComponent, SetProperty, SetBehavior,
                 DeleteBehavior, AddGraphNode, DeleteGraphNode, ConnectPorts, DisconnectPorts,
                 AddSceneNode, DeleteSceneNode, SetSceneParent, RenameSceneNode, OrderSceneNode, SetSceneTransform, PlacePrefab,
                 SetPrefabParameter, SetComponentOverride, SetParameterOverride>;

struct EditReceipt {
    std::uint64_t revision = 0;
    std::vector<runtime::AssetId> changed_documents;
};

// A transaction is validated and published once. No caller receives a mutable
// document, and an error in any command leaves the entire project untouched.
struct EditRequest {
    std::uint64_t expected_revision = 0;
    std::vector<Command> commands;
};

} // namespace epidemic::game::editor
