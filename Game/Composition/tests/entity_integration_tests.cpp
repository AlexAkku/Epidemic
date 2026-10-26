#include "test_support.h"

using namespace composition_tests;
void Components();
void GraphBehaviors();
void Systems();
void EntityIntegration() {
    Fixture f;
    const auto entity = f.Entity();
    auto scope = Take(gc::GraphScope::ForEntity(f.entities, entity));
    Check(f.bridge.Enqueue(bridge::MaterializeRequest{scope->Object(), bridge::RuntimePersistentObjectHandle{1}}).HasValue(), "queue existing RuntimeBridge projection");
    (void)f.bridge.Process();
    std::size_t calls = 0;
    const auto bindings = Bindings([&](const gc::OperationContext &context, std::int64_t value) {
        Check(context.scope.Object() == fw::entities::EntityService::ToGameplayObjectRef(entity), "correct Entity context");
        Check(context.bridge.GetBinding(context.scope.Object()).has_value(), "native operation resolves existing RuntimeBridge binding");
        ++calls; return foundation::Result<std::int64_t>::Success(value);
    });
    const auto bound = Take(f.host.Compile(scope, Definition(), bindings));
    gc::GraphExecutionHost foreign{f.scheduler, f.bridge};
    ErrorIs(foreign.Start(bound, Id("event.in"), 1), "composition.foreign_program");
    const auto first = Take(f.host.Start(bound, Id("event.in"), 1));
    const auto second = Take(f.host.Start(bound, Id("event.in"), 1));
    f.Tick(); Check(calls == 2 && Status(*first) == graph::ExecutionStatus::Waiting, "two owned prefixes wait");
    Check(f.entities.RequestDestroy(entity, fw::entities::EntityDestroyReason::Destroyed).HasValue(), "destroy existing Entity");
    f.Tick(); f.Advance(); f.Tick();
    Check(calls == 2 && Status(*first) == graph::ExecutionStatus::Cancelled && Status(*second) == graph::ExecutionStatus::Cancelled, "Entity destruction cancels all continuations");
    ErrorIs(f.host.Start(bound, Id("event.in"), 1), "composition.owner_expired");
    Check(f.entities.CommitPendingDestruction().HasValue(), "commit destruction");
    const auto next = f.Entity();
    auto next_scope = Take(gc::GraphScope::ForEntity(f.entities, next));
    Check(!scope->IsAlive() && next_scope->IsAlive(), "generation protects slot reuse");
    const auto self_bindings = Bindings([&](const gc::OperationContext &, std::int64_t value) {
        ++calls;
        Check(f.entities.RequestDestroy(next, fw::entities::EntityDestroyReason::Destroyed).HasValue(), "native operation destroys its Entity");
        return foundation::Result<std::int64_t>::Success(value);
    });
    const auto self_program = Take(f.host.Compile(next_scope, Definition(), self_bindings));
    auto self = Take(f.host.Start(self_program, Id("event.in"), 0)); f.Tick();
    Check(Status(*self) == graph::ExecutionStatus::Cancelled && calls == 3, "native destruction stops before Wait and suffix");
    const auto part_entity = f.Entity();
    auto part = Take(gc::GraphScope::ForEntity(f.entities, part_entity, fw::entities::EntityPartId::FromString("game.body")));
    Check(part->Part().IsValid(), "existing Entity Parts are supported");
    ErrorIs(gc::GraphScope::ForEntity(f.entities, part_entity, fw::entities::EntityPartId::FromString("missing")), "composition.invalid_entity");
}
int main() {
    try { EntityIntegration(); Components(); GraphBehaviors(); Systems(); }
    catch (const std::exception &error) { std::fprintf(stderr, "%s\n", error.what()); return 1; }
    return 0;
}
