#include "Epidemic/Runtime/Schema/native_schema.h"
#include <cstdint>
#include <cstdio>
#include <iostream>
#include <stdexcept>

using namespace epidemic;
using namespace epidemic::runtime::schema;
namespace {
struct Probe {
  std::int32_t value{};
};
struct Other {
  std::int32_t value{};
};
bool Valid(const std::int32_t &value) { return value >= 0 && value <= 100; }
foundation::Result<std::int32_t> Plus(Probe &probe,
                                      const std::int32_t &amount) {
  if (amount < 0 || amount > 100 || !Valid(probe.value))
    return foundation::Result<std::int32_t>::Failure(
        foundation::Error::Create("probe.range", "out of range"));
  return foundation::Result<std::int32_t>::Success(probe.value + amount);
}
void Check(bool value, const char *message) {
  if (!value) {
    std::fputs(message, stderr);
    std::fputc('\n', stderr);
    throw std::runtime_error(message);
  }
}
template <typename T>
void ErrorIs(const foundation::Result<T> &value, std::string_view code) {
  Check(!value && value.GetError().HasCode(code), "unexpected schema failure");
}
int Run() {
  NativeSchemaRegistry registry;
  const auto registered = registry.RegisterType<Probe>("probe.native");
  Check(registered.HasValue(), "register native type");
  const auto type = registered.Value();
  const auto property =
      registry.RegisterProperty(type, "value", &Probe::value, &Valid);
  const auto operation = registry.RegisterOperation(type, "plus", &Plus);
  Check(property && operation, "register bindings");
  const auto *metadata = registry.FindType(type);
  Check(metadata && metadata->canonical_name == "probe.native" &&
            metadata->cpp_type == typeid(Probe),
        "metadata");
  Probe object;
  Check(registry.Read<std::int32_t>(type, property.Value(), object).Value() ==
            0,
        "read");
  Check(registry.Write(type, property.Value(), object, std::int32_t{7})
            .HasValue(),
        "write");
  Check(registry.Invoke<std::int32_t>(type, operation.Value(), object,
                                      std::int32_t{5})
                .Value() == 12,
        "invoke");

  ErrorIs(registry.Write(type, property.Value(), object, std::int32_t{-1}),
          "schema.value_rejected");
  Check(object.value == 7, "failed validation mutated object");
  ErrorIs(registry.Read<double>(type, property.Value(), object),
          "schema.type_mismatch");
  ErrorIs(registry.Write(type, property.Value(), object, 2.5),
          "schema.type_mismatch");
  Other other;
  ErrorIs(registry.Read<std::int32_t>(type, property.Value(), other),
          "schema.type_mismatch");
  ErrorIs(
      registry.Invoke<double>(type, operation.Value(), object, std::int32_t{5}),
      "schema.type_mismatch");
  ErrorIs(registry.Invoke<std::int32_t>(type, operation.Value(), object, 5.0),
          "schema.type_mismatch");
  ErrorIs(registry.Invoke<std::int32_t>(type, operation.Value(), object,
                                        std::int32_t{-1}),
          "probe.range");
  const auto missing = SchemaId::FromString("missing");
  ErrorIs(registry.Read<std::int32_t>(missing, property.Value(), object),
          "schema.type_not_found");
  ErrorIs(registry.Read<std::int32_t>(type, missing, object),
          "schema.member_not_found");
  ErrorIs(registry.Invoke<std::int32_t>(type, missing, object, std::int32_t{1}),
          "schema.member_not_found");
  Check(registry.FindType(missing) == nullptr, "unknown metadata");
  ErrorIs(registry.RegisterType<Probe>("probe.native"),
          "schema.duplicate_type");
  ErrorIs(registry.RegisterProperty(type, "value", &Probe::value),
          "schema.duplicate_member");
  ErrorIs(registry.RegisterOperation(type, "plus", &Plus),
          "schema.duplicate_member");
  ErrorIs(registry.RegisterProperty(type, "bad", &Other::value),
          "schema.type_mismatch");
  ErrorIs(registry.RegisterType<Probe>(""), "schema.invalid_name");
  ErrorIs(registry.RegisterProperty(
              type, "null", static_cast<std::int32_t Probe::*>(nullptr)),
          "schema.invalid_descriptor");
  Check(object.value == 7, "failed operations changed owner");
  std::cout << "Native schema PASS\n";
  return 0;
}
} // namespace

int main() {
  try {
    return Run();
  } catch (...) {
    std::fputs("Native schema test failed\n", stderr);
  }
  return 1;
}
