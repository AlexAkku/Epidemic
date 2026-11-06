#include "renderer.h"
#include <jawt.h>
#include <jawt_md.h>
#include <jni.h>
#include <psapi.h>
#include <tlhelp32.h>
#include <algorithm>
#include <atomic>
#include <cstdio>
#include <cstdint>
#include <map>
#include <memory>
#include <stdexcept>
#include <string>

namespace {
using epidemic::runtime::presentation::Renderer;

struct Surface {
    JAWT awt{};
    JAWT_DrawingSurface *surface = nullptr;
    JAWT_DrawingSurfaceInfo *info = nullptr;
    bool locked = false;

    Surface(JNIEnv *env, jobject canvas) {
        awt.version = JAWT_VERSION_1_4;
        if (!JAWT_GetAWT(env, &awt))
            return;
        surface = awt.GetDrawingSurface(env, canvas);
        if (!surface)
            return;
        locked = (static_cast<unsigned>(surface->Lock(surface)) &
                  static_cast<unsigned>(JAWT_LOCK_ERROR)) == 0;
        if (locked)
            info = surface->GetDrawingSurfaceInfo(surface);
    }

    ~Surface() {
        if (info)
            surface->FreeDrawingSurfaceInfo(info);
        if (locked)
            surface->Unlock(surface);
        if (surface)
            awt.FreeDrawingSurface(surface);
    }

    Surface(const Surface &) = delete;
    Surface &operator=(const Surface &) = delete;

    HWND Window() const {
        if (!info || !info->platformInfo)
            throw std::runtime_error("JAWT drawing surface unavailable");
        return static_cast<JAWT_Win32DrawingSurfaceInfo *>(info->platformInfo)->hwnd;
    }
};

struct View {
    DWORD thread = GetCurrentThreadId();
    HWND hwnd = nullptr;
    std::unique_ptr<Renderer> renderer;
};

// Diagnostic scoped surface IDs, never C++ addresses or authoring identities.
std::map<jlong, View> &Views() {
    // Initialization is inside the JNI callers' exception boundaries.
    static std::map<jlong, View> value;
    return value;
}

jlong next_id = 1;
std::atomic<DWORD> owner_thread{0};

void CheckThread() {
    const DWORD current = GetCurrentThreadId();
    DWORD expected = 0;
    if (!owner_thread.compare_exchange_strong(expected, current) && expected != current)
        throw std::runtime_error("Native probe must run on its original EDT");
}

jstring Reply(JNIEnv *env, const std::string &value) {
    return env->NewStringUTF(value.c_str());
}

jstring Failure(JNIEnv *env, const char *message) noexcept {
    // Catch handlers must not allocate std::string and throw across the JNI ABI.
    char text[768]{};
    std::snprintf(text, sizeof(text), "FAIL %.700s", message);
    return env->NewStringUTF(text);
}

std::uint64_t Number(HWND hwnd) {
    return static_cast<std::uint64_t>(reinterpret_cast<std::uintptr_t>(hwnd));
}
} // namespace

extern "C" JNIEXPORT jstring JNICALL Java_epidemic_u1_NativeViewport_frame(
    JNIEnv *env, jclass, jlong id, jobject canvas, jboolean retain_capture) noexcept {
    try {
        CheckThread();
        auto &views = Views();
        Surface surface(env, canvas);
        const HWND hwnd = surface.Window();
        if (!IsWindow(hwnd))
            throw std::runtime_error("JAWT returned invalid HWND");
        bool inserted = false;
        if (id == 0) {
            id = next_id++;
            views.try_emplace(id);
            inserted = true;
        }
        auto found = views.find(id);
        if (found == views.end())
            throw std::runtime_error("Stale viewport handle");
        try {
            auto &view = found->second;
            if (view.thread != GetCurrentThreadId())
                throw std::runtime_error("Viewport thread mismatch");
            if (view.hwnd != hwnd) {
                view.renderer.reset();
                view.hwnd = hwnd;
            }
            if (!view.renderer)
                view.renderer = std::make_unique<
                    Renderer>(hwnd, std::span<const epidemic::runtime::presentation::SpriteEntry>{},
                              0);
            view.renderer->EnableCaptures(retain_capture == JNI_TRUE);
            view.renderer->Resize();
            const auto size = view.renderer->ClientExtent();
            if (size.x > 0 && size.y > 0) {
                const auto logical = view.renderer->LogicalExtent();
                const float base = size.x / logical.x;
                const float offset_x = (size.x - std::min(1280.f, size.x) * base) * .5f;
                const float offset_y = (size.y - 720.f * base) * .5f;
                view.renderer->Begin();
                // Same native editor surface integration: remove gameplay letterboxing.
                view.renderer->PushTransform(
                    D2D1::Matrix3x2F::Scale(1 / base, 1 / base) *
                    D2D1::Matrix3x2F::Translation(-offset_x / base, -offset_y / base));
                view.renderer->Rect(0, 0, size.x, size.y, 0x191920);
                for (unsigned x = 0; x < static_cast<unsigned>(size.x); x += 48)
                    view.renderer->Line(static_cast<float>(x), 0, static_cast<float>(x), size.y,
                                        0x363640);
                for (unsigned y = 0; y < static_cast<unsigned>(size.y); y += 48)
                    view.renderer->Line(0, static_cast<float>(y), size.x, static_cast<float>(y),
                                        0x363640);
                view.renderer->RoundRect(size.x / 2 - 50, size.y / 2 - 36, 100, 72, 6, 0xa855d5);
                view.renderer->Text(L"U1 / existing D3D11 + D2D Renderer", 20, 20, 16, 0xf0f0f4,
                                    size.x - 40);
                view.renderer->PopTransform();
                view.renderer->End();
            }
            return Reply(env, "PASS " + std::to_string(id) + " " + std::to_string(Number(hwnd)) +
                                  " " + std::to_string(Number(GetAncestor(hwnd, GA_ROOT))));
        } catch (...) {
            if (inserted)
                views.erase(id);
            throw;
        }
    } catch (const std::exception &error) {
        return Failure(env, error.what());
    } catch (...) {
        return Failure(env, "unknown native exception");
    }
}

extern "C" JNIEXPORT jstring JNICALL Java_epidemic_u1_NativeViewport_release(JNIEnv *env, jclass,
                                                                             jlong id) noexcept {
    try {
        CheckThread();
        auto &views = Views();
        if (id != 0 && views.erase(id) != 1)
            return Reply(env, "FAIL stale viewport release");
        return Reply(env, "PASS released");
    } catch (const std::exception &error) {
        return Failure(env, error.what());
    } catch (...) {
        return Failure(env, "unknown release exception");
    }
}

extern "C" JNIEXPORT jstring JNICALL
Java_epidemic_u1_NativeViewport_capture(JNIEnv *env, jclass, jlong id, jstring path) noexcept {
    try {
        CheckThread();
        auto &views = Views();
        const auto found = views.find(id);
        if (found == views.end() || !found->second.renderer)
            return Reply(env, "FAIL stale capture surface");
        if (!path)
            return Reply(env, "FAIL missing capture path");
        const auto *characters = env->GetStringChars(path, nullptr);
        if (!characters)
            return nullptr; // Preserve the JVM's allocation failure.
        std::wstring filename;
        try {
            filename.assign(reinterpret_cast<const wchar_t *>(characters),
                            static_cast<std::size_t>(env->GetStringLength(path)));
        } catch (...) {
            env->ReleaseStringChars(path, characters);
            throw;
        }
        env->ReleaseStringChars(path, characters);
        found->second.renderer->Capture(std::filesystem::path{filename});
        return Reply(env, "PASS GPU capture");
    } catch (const std::exception &error) {
        return Failure(env, error.what());
    } catch (...) {
        return Failure(env, "unknown capture exception");
    }
}

extern "C" JNIEXPORT jlongArray JNICALL Java_epidemic_u1_NativeViewport_metrics(JNIEnv *env,
                                                                                jclass) noexcept {
    try {
        CheckThread();
        // No application model, native thread or callback into Java is created.
        PROCESS_MEMORY_COUNTERS_EX memory{};
        memory.cb = sizeof(memory);
        GetProcessMemoryInfo(GetCurrentProcess(),
                             reinterpret_cast<PROCESS_MEMORY_COUNTERS *>(&memory), sizeof(memory));
        DWORD handles = 0;
        GetProcessHandleCount(GetCurrentProcess(), &handles);
        DWORD threads = 0;
        const HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
        if (snapshot != INVALID_HANDLE_VALUE) {
            THREADENTRY32 entry{};
            entry.dwSize = sizeof(entry);
            if (Thread32First(snapshot, &entry)) {
                do {
                    if (entry.th32OwnerProcessID == GetCurrentProcessId())
                        ++threads;
                } while (Thread32Next(snapshot, &entry));
            }
            CloseHandle(snapshot);
        }
        const jlong values[]{static_cast<jlong>(Views().size()),
                             static_cast<jlong>(handles),
                             static_cast<jlong>(
                                 GetGuiResources(GetCurrentProcess(), GR_GDIOBJECTS)),
                             static_cast<jlong>(memory.WorkingSetSize),
                             static_cast<jlong>(memory.PrivateUsage),
                             static_cast<jlong>(threads)};
        auto result = env->NewLongArray(6);
        if (result)
            env->SetLongArrayRegion(result, 0, 6, values);
        return result;
    } catch (...) {
        return env->NewLongArray(0); // Explicit failed diagnostic result, no C++ escape.
    }
}

extern "C" JNIEXPORT jlong JNICALL
Java_epidemic_u1_NativeViewport_windowHandle(JNIEnv *env, jclass, jobject component) noexcept {
    try {
        CheckThread();
        Surface surface(env, component);
        const HWND root = GetAncestor(surface.Window(), GA_ROOT);
        return IsWindow(root) ? static_cast<jlong>(Number(root)) : 0;
    } catch (...) {
        return 0; // Explicit unavailable diagnostic, no borrowed native handle escapes.
    }
}
