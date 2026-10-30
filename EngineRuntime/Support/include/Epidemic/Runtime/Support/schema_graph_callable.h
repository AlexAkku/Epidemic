#pragma once

#include "Epidemic/Runtime/Graph/graph_compiler.h"
#include "Epidemic/Runtime/Schema/native_schema.h"
#include <concepts>
#include <type_traits>

namespace epidemic::runtime {
namespace support_detail {
template <std::copy_constructible Owner>
class SchemaGraphFactory final : public graph::INativeCallableFactory {
    class Invocation final : public graph::INativeInvocation {
      public:
        Invocation(std::shared_ptr<const schema::NativeSchemaRegistry> registry,
                   schema::NativeOperationRef operation, const Owner &initial)
            : registry_(std::move(registry)), operation_(operation), owner_(initial) {
        }

        foundation::Result<std::int64_t> Invoke(std::int64_t argument) override {
            return registry_->Invoke<std::int64_t>(operation_.owner_type, operation_.operation,
                                                   owner_, argument);
        }

      private:
        std::shared_ptr<const schema::NativeSchemaRegistry> registry_;
        schema::NativeOperationRef operation_;
        Owner owner_;
    };

  public:
    SchemaGraphFactory(std::shared_ptr<const schema::NativeSchemaRegistry> registry,
                       schema::NativeOperationRef operation, Owner initial)
        : registry_(std::move(registry)), operation_(operation), initial_(std::move(initial)) {
    }

    foundation::Result<std::unique_ptr<graph::INativeInvocation>>
    CreateInvocation() const override {
        return foundation::Result<std::unique_ptr<graph::INativeInvocation>>::Success(
            std::make_unique<Invocation>(registry_, operation_, initial_));
    }

  private:
    std::shared_ptr<const schema::NativeSchemaRegistry> registry_;
    schema::NativeOperationRef operation_;
    const Owner initial_;
};

} // namespace support_detail

// The initial owner is copied per call site per instance. Owner copy semantics
// must isolate its mutable data; external shared objects are caller-owned.
// Registry access retains its existing external-synchronization contract.
template <std::copy_constructible Owner>
    requires std::is_nothrow_destructible_v<Owner>
[[nodiscard]] foundation::Result<std::shared_ptr<const graph::INativeCallableFactory>>
CreateSchemaGraphCallable(std::shared_ptr<const schema::NativeSchemaRegistry> registry,
                          schema::NativeOperationRef operation, Owner initial) {
    using Result = foundation::Result<std::shared_ptr<const graph::INativeCallableFactory>>;
    if (!registry)
        return Result::Failure(
            foundation::Error::Create("graph.native.invalid_registry", "registry is required"));
    const auto valid = registry->CheckOperation<Owner, std::int64_t, std::int64_t>(operation);
    if (!valid)
        return Result::Failure(valid.GetError());
    return Result::Success(
        std::make_shared<support_detail::SchemaGraphFactory<Owner>>(std::move(registry), operation,
                                                                    std::move(initial)));
}

} // namespace epidemic::runtime
