#include <Windows.h>
#include <shellapi.h>
#include <jni.h>
#include "include/cef_sandbox_win.h"
#include "include/cef_version.h"
#include <array>
#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <format>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

std::string InspectSandbox();
namespace {
void *sandbox_info = nullptr;
std::filesystem::path ModulePath() {
    std::array<wchar_t, 32768> buffer{};
    const auto size = GetModuleFileNameW(nullptr, buffer.data(), static_cast<DWORD>(buffer.size()));
    if (!size || size == buffer.size()) throw std::runtime_error("launcher.module_path");
    return std::wstring{buffer.data(), size};
}
std::string Utf8(std::wstring_view text) {
    if (text.empty()) return {};
    const int size = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, text.data(), static_cast<int>(text.size()), nullptr, 0, nullptr, nullptr);
    if (!size) throw std::runtime_error("launcher.unicode");
    std::string result(static_cast<std::size_t>(size), '\0');
    WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, text.data(), static_cast<int>(text.size()), result.data(), size, nullptr, nullptr);
    return result;
}
jboolean JNICALL SandboxCreated(JNIEnv *, jclass) { return sandbox_info != nullptr; }
jstring JNICALL Security(JNIEnv *env, jclass) {
    try { return env->NewStringUTF(InspectSandbox().c_str()); }
    catch (...) { return env->NewStringUTF("{\"error\":\"launcher.security_query\"}"); }
}
std::string JavaError(JNIEnv *env) {
    auto error = env->ExceptionOccurred();
    env->ExceptionClear();
    if (!error) return "launcher.java_entry";
    const auto type = env->GetObjectClass(error);
    const auto method = env->GetMethodID(type, "toString", "()Ljava/lang/String;");
    const auto text = static_cast<jstring>(env->CallObjectMethod(error, method));
    const auto bytes = env->GetStringUTFChars(text, nullptr);
    std::string result(bytes ? bytes : "launcher.java_exception");
    if (bytes) env->ReleaseStringUTFChars(text, bytes);
    return result;
}
int RunJava(const std::filesystem::path &home) {
    const auto runtime = home / L"runtime";
    const auto jvm = LoadLibraryExW((runtime / L"bin/server/jvm.dll").c_str(), nullptr, LOAD_WITH_ALTERED_SEARCH_PATH);
    if (!jvm) throw std::runtime_error("launcher.jvm_load");
    using CreateVM = jint(JNICALL *)(JavaVM **, void **, void *);
    const auto create_vm = reinterpret_cast<CreateVM>(GetProcAddress(jvm, "JNI_CreateJavaVM"));
    if (!create_vm) throw std::runtime_error("launcher.jvm_entry");
    std::vector<std::string> values;
    std::ifstream options(home / L"app/launcher-options.txt", std::ios::binary);
    if (!options) throw std::runtime_error("launcher.options_missing");
    for (std::string line; std::getline(options, line);) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (line.size() > 8192 || values.size() > 256) throw std::runtime_error("launcher.options_limit");
        if (!line.empty()) values.push_back(std::move(line));
    }
    // The JNI invocation API does not perform the Java launcher's wildcard /
    // manifest classpath expansion. Supply every locked product JAR explicitly.
    std::vector<std::filesystem::path> jars;
    for (const auto &entry : std::filesystem::directory_iterator(home / L"app/lib"))
        if (entry.path().extension() == L".jar") jars.push_back(entry.path());
    std::sort(jars.begin(), jars.end());
    // Product bootstrap corrections (DirectoryLock/MainImpl) must precede the
    // corresponding upstream classes, just as with jpackage's main JAR.
    const auto primary = home / L"app/lib/epidemic.jar";
    if (!std::filesystem::is_regular_file(primary)) throw std::runtime_error("launcher.product_jar_missing");
    std::string classpath = Utf8(primary.wstring());
    for (const auto &jar : jars) {
        if (jar == primary) continue;
        classpath += ';';
        classpath += Utf8(jar.wstring());
    }
    values.push_back("-Djava.class.path=" + classpath);
    values.push_back("-Djcef.remote.enabled=false");
    values.push_back("-Djcef.sandbox.ptr=" + std::format("{:016X}", reinterpret_cast<std::uintptr_t>(sandbox_info)));
    values.push_back("-Djcef.sandbox.cefVersion=" CEF_VERSION);
    std::vector<JavaVMOption> vm_options;
    for (auto &value : values) vm_options.push_back({value.data(), nullptr});
    JavaVMInitArgs init{JNI_VERSION_1_8, static_cast<jint>(vm_options.size()), vm_options.data(), JNI_FALSE};
    JavaVM *vm = nullptr;
    JNIEnv *env = nullptr;
    if (create_vm(&vm, reinterpret_cast<void **>(&env), &init) != JNI_OK)
        throw std::runtime_error("launcher.jvm_create");
    const auto native = env->FindClass("epidemic/launcher/NativeLauncher");
    if (!native) throw std::runtime_error(JavaError(env));
    JNINativeMethod methods[]{
        {const_cast<char *>("sandboxCreated"), const_cast<char *>("()Z"), reinterpret_cast<void *>(&SandboxCreated)},
        {const_cast<char *>("security"), const_cast<char *>("()Ljava/lang/String;"), reinterpret_cast<void *>(&Security)}
    };
    if (env->RegisterNatives(native, methods, 2) != JNI_OK) throw std::runtime_error(JavaError(env));
    int count = 0;
    const auto argv = CommandLineToArgvW(GetCommandLineW(), &count);
    if (!argv) throw std::runtime_error("launcher.arguments");
    const auto string_class = env->FindClass("java/lang/String");
    const auto args = env->NewObjectArray(count - 1, string_class, nullptr);
    for (int i = 1; i < count; ++i) {
        const auto value = env->NewString(reinterpret_cast<const jchar *>(argv[i]), static_cast<jsize>(wcslen(argv[i])));
        env->SetObjectArrayElement(args, i - 1, value);
        env->DeleteLocalRef(value);
    }
    LocalFree(static_cast<HLOCAL>(argv));
    const auto loader_class = env->FindClass("java/lang/ClassLoader");
    const auto loader = env->CallStaticObjectMethod(loader_class,
        env->GetStaticMethodID(loader_class, "getSystemClassLoader", "()Ljava/lang/ClassLoader;"));
    const auto thread_class = env->FindClass("java/lang/Thread");
    const auto thread = env->CallStaticObjectMethod(thread_class,
        env->GetStaticMethodID(thread_class, "currentThread", "()Ljava/lang/Thread;"));
    env->CallVoidMethod(thread, env->GetMethodID(thread_class, "setContextClassLoader", "(Ljava/lang/ClassLoader;)V"), loader);
    if (env->ExceptionCheck()) throw std::runtime_error(JavaError(env));
    const auto main = env->FindClass("epidemic/launcher/Main");
    const auto method = main ? env->GetStaticMethodID(main, "main", "([Ljava/lang/String;)V") : nullptr;
    if (!method) throw std::runtime_error(JavaError(env));
    env->CallStaticVoidMethod(main, method, args);
    const bool failed = env->ExceptionCheck();
    if (failed) throw std::runtime_error(JavaError(env));
    vm->DestroyJavaVM();
    return failed ? 2 : 0;
}
}
int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, PWSTR, int) {
    // This object is linked into the main EXE and precedes JVM/CEF initialization.
    CefScopedSandboxInfo sandbox;
    sandbox_info = sandbox.sandbox_info();
    try {
        if (!sandbox_info) throw std::runtime_error("launcher.sandbox_create");
        const auto home = ModulePath().parent_path();
        const auto bin = home / L"runtime/bin";
        SetDefaultDllDirectories(LOAD_LIBRARY_SEARCH_DEFAULT_DIRS);
        AddDllDirectory(bin.c_str());
        // JetBrains' public helper exports the renderer/router implementation.
        // Chromium subprocesses use this same EXE and never create a JVM.
        if (std::wstring_view(GetCommandLineW()).find(L"--type=") != std::wstring_view::npos) {
            const auto helper = LoadLibraryExW((bin / L"jcef_helper.dll").c_str(), nullptr, LOAD_WITH_ALTERED_SEARCH_PATH);
            if (!helper) throw std::runtime_error("launcher.cef_helper_load");
            using Execute = int(WINAPI *)(void *, void *);
            const auto execute = reinterpret_cast<Execute>(GetProcAddress(helper, "execute_subprocess"));
            if (!execute) throw std::runtime_error("launcher.cef_helper_entry");
            return execute(static_cast<void *>(&instance), sandbox_info);
        }
        return RunJava(home);
    } catch (const std::exception &error) {
        // No modal error dialog can leave a failed unattended launch hanging.
        try { std::ofstream(ModulePath().parent_path() / L"launcher-error.log", std::ios::app) << error.what() << '\n'; }
        catch (...) { OutputDebugStringA("Epidemic launcher: startup diagnostic could not be written"); }
        return 2;
    }
}
