#include "Epidemic/Runtime/Schema/native_schema.h"
#include <utility>

namespace epidemic::runtime::schema {
namespace {
bool ValidName(std::string_view name) {
  if (name.empty())
    return false;
  for (const char c : name) {
    if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
          (c >= '0' && c <= '9') || c == '.' || c == '_' || c == '-'))
      return false;
  }
  return true;
}
} // namespace

foundation::Result<SchemaId>
NativeSchemaRegistry::RegisterNativeType(std::string_view name,
                                         std::type_index owner) {
  if (!ValidName(name))
    return Failure<SchemaId>("schema.invalid_name");
  const auto id = SchemaId::FromString(name);
  if (const auto found = types_.find(id); found != types_.end())
    return Failure<SchemaId>(found->second.info.canonical_name == name
                                 ? "schema.duplicate_type"
                                 : "schema.id_collision");
  types_.try_emplace(id, NativeTypeInfo{id, std::string(name), owner});
  return foundation::Result<SchemaId>::Success(id);
}

foundation::Result<SchemaId>
NativeSchemaRegistry::RegisterMember(SchemaId type, std::type_index owner,
                                     std::string_view name, std::any binding,
                                     bool operation) {
  const auto found = types_.find(type);
  if (found == types_.end())
    return Failure<SchemaId>("schema.type_not_found");
  if (found->second.info.cpp_type != owner)
    return Failure<SchemaId>("schema.type_mismatch");
  if (!ValidName(name))
    return Failure<SchemaId>("schema.invalid_name");
  auto &members =
      operation ? found->second.operations : found->second.properties;
  const auto id = SchemaId::FromString(name);
  if (const auto existing = members.find(id); existing != members.end())
    return Failure<SchemaId>(existing->second.canonical_name == name
                                 ? "schema.duplicate_member"
                                 : "schema.id_collision");
  members.emplace(id, Member{std::string(name), std::move(binding)});
  return foundation::Result<SchemaId>::Success(id);
}

const NativeTypeInfo *
NativeSchemaRegistry::FindType(SchemaId type) const noexcept {
  const auto found = types_.find(type);
  return found == types_.end() ? nullptr : &found->second.info;
}

foundation::Result<const std::any *>
NativeSchemaRegistry::FindBinding(MemberKey key, std::type_index owner,
                                  bool operation) const {
  const auto found = types_.find(key.type);
  if (found == types_.end())
    return Failure<const std::any *>("schema.type_not_found");
  if (found->second.info.cpp_type != owner)
    return Failure<const std::any *>("schema.type_mismatch");
  const auto &members =
      operation ? found->second.operations : found->second.properties;
  const auto binding = members.find(key.member);
  if (binding == members.end())
    return Failure<const std::any *>("schema.member_not_found");
  return foundation::Result<const std::any *>::Success(
      &binding->second.binding);
}
} // namespace epidemic::runtime::schema
