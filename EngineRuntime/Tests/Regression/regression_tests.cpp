#include "Epidemic/Runtime/Support/runtime_support.h"

#include <Epidemic/Core/application.h>

#include <iostream>
#include <cstdio>
#include <string_view>

namespace {
bool Expect(bool condition, std::string_view message) {
    if (!condition) {
        std::cerr << message << '\n';
    }
    return condition;
}
} // namespace

int RunRegressionTests() {
    epidemic::core::Application app{};
    epidemic::runtime::EngineRuntimeOptions options{};
    options.enable_scene = false;

    const auto result = epidemic::runtime::RegisterDefaultEngineRuntime(app, options);
    bool ok = Expect(!result.HasValue(), "preflight should reject invalid default composition");
    ok &= Expect(!app.Services().Contains<epidemic::runtime::RuntimeFoundationRegistration>(),
                 "preflight failure must not partially register services");
    return ok ? 0 : 1;
}

int main() {
    try { return RunRegressionTests(); }
    catch (...) { std::fputs("Regression tests threw\n", stderr); return 1; }
}
