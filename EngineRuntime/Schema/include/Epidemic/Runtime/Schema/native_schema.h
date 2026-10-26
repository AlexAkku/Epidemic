#pragma once

#include "Epidemic/Foundation/result.h"
#include "Epidemic/Foundation/string_id.h"

#include <any>
#include <string>
#include <string_view>
#include <typeindex>
#include <unordered_map>
#include <utility>

namespace epidemic::runtime::schema {
using SchemaId = foundation::StringId;

struct NativeOperationRef {
    SchemaId owner_type{};
    SchemaId operation{};
};

struct NativeTypeInfo {
    SchemaId id;
    std::string canonical_name;
    std::type_index cpp_type;
};

// Runtime cannot depend on Framework's StableTypeRegistry. Reuse Foundation IDs
// and Result; add only native member bindings. No new persistent identity system.
// Externally synchronized. Objects are borrowed for a call, never retained.
// RTTI/bindings are process-local metadata, not a wire format or managed ABI.
class NativeSchemaRegistry {
  public:
    template <typename Owner> foundation::Result<SchemaId> RegisterType(std::string_view name) {
        return RegisterNativeType(name, typeid(Owner));
    }

    template <typename Owner, typename Value>
    foundation::Result<SchemaId> RegisterProperty(SchemaId type, std::string_view name, Value Owner::*member,
                                                bool (*validate)(const Value &) = nullptr) {
        if (!member) return Failure<SchemaId>("schema.invalid_descriptor");
        return RegisterMember(type, typeid(Owner), name, Property<Owner, Value>{member, validate}, false);
    }

    template <typename Owner, typename Argument, typename Return>
    foundation::Result<SchemaId> RegisterOperation(SchemaId type, std::string_view name,
        foundation::Result<Return> (*function)(Owner &, const Argument &)) {
        if (!function) return Failure<SchemaId>("schema.invalid_descriptor");
        return RegisterMember(type, typeid(Owner), name, Operation<Owner, Argument, Return>{function}, true);
    }

    [[nodiscard]] const NativeTypeInfo *FindType(SchemaId type) const noexcept;

    // Check a callable signature without executing it or mutating an owner.
    template <typename Owner, typename Argument, typename Return>
    [[nodiscard]] foundation::Result<void> CheckOperation(NativeOperationRef reference) const {
        const auto binding = Resolve<Operation<Owner, Argument, Return>>(
            reference.owner_type, reference.operation, typeid(Owner), true);
        if (!binding) return foundation::Result<void>::Failure(binding.GetError());
        return foundation::Result<void>::Success();
    }

    template <typename Value, typename Owner>
    [[nodiscard]] foundation::Result<Value> Read(SchemaId type, SchemaId property, const Owner &object) const {
        const auto binding = Resolve<Property<Owner, Value>>(type, property, typeid(Owner), false);
        if (!binding) return foundation::Result<Value>::Failure(binding.GetError());
        return foundation::Result<Value>::Success(object.*(binding.Value()->member));
    }

    template <typename Owner, typename Value>
    [[nodiscard]] foundation::Result<void> Write(SchemaId type, SchemaId property, Owner &object,
                                                const Value &value) const {
        const auto binding = Resolve<Property<Owner, Value>>(type, property, typeid(Owner), false);
        if (!binding) return foundation::Result<void>::Failure(binding.GetError());
        if (binding.Value()->validate && !binding.Value()->validate(value))
            return Failure<void>("schema.value_rejected");
        object.*(binding.Value()->member) = value;
        return foundation::Result<void>::Success();
    }

    template <typename Return, typename Owner, typename Argument>
    [[nodiscard]] foundation::Result<Return> Invoke(SchemaId type, SchemaId operation, Owner &object,
                                                   const Argument &argument) const {
        const auto binding = Resolve<Operation<Owner, Argument, Return>>(type, operation, typeid(Owner), true);
        if (!binding) return foundation::Result<Return>::Failure(binding.GetError());
        return binding.Value()->function(object, argument);
    }

  private:
    template <typename Owner, typename Value> struct Property {
        Value Owner::*member;
        bool (*validate)(const Value &);
    };
    template <typename Owner, typename Argument, typename Return> struct Operation {
        foundation::Result<Return> (*function)(Owner &, const Argument &);
    };
    struct Member {
        std::string canonical_name;
        std::any binding;
    };
    struct Record {
        explicit Record(NativeTypeInfo type) : info(std::move(type)) {}
        NativeTypeInfo info;
        std::unordered_map<SchemaId, Member> properties;
        std::unordered_map<SchemaId, Member> operations;
    };

    template <typename Value> static foundation::Result<Value> Failure(std::string_view code) {
        return foundation::Result<Value>::Failure(foundation::Error::Create(code, "native schema contract rejected"));
    }
    foundation::Result<SchemaId> RegisterNativeType(std::string_view name, std::type_index owner);
    foundation::Result<SchemaId> RegisterMember(SchemaId type, std::type_index owner, std::string_view name,
                                               std::any binding, bool operation);
    struct MemberKey { SchemaId type; SchemaId member; };
    foundation::Result<const std::any *> FindBinding(MemberKey key, std::type_index owner, bool operation) const;
    template <typename Binding>
    foundation::Result<const Binding *> Resolve(SchemaId type, SchemaId member, std::type_index owner,
                                                bool operation) const {
        const auto erased = FindBinding({type, member}, owner, operation);
        if (!erased) return foundation::Result<const Binding *>::Failure(erased.GetError());
        const auto *binding = std::any_cast<Binding>(erased.Value());
        if (!binding) return Failure<const Binding *>("schema.type_mismatch");
        return foundation::Result<const Binding *>::Success(binding);
    }
    std::unordered_map<SchemaId, Record> types_;
};
} // namespace epidemic::runtime::schema
