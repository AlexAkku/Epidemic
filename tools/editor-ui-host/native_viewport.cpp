#include "renderer.h"
#include "native_scene_preview.h"
#include "Epidemic/Game/Editor/scene_drag.h"
#include <cmath>
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
#include <limits>
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
    std::unique_ptr<epidemic::editor_host::ScenePreview> scene;
    epidemic::game::editor::SceneView camera;
    epidemic::runtime::SceneNodeId selected{};
    bool editing_scene = false;
    std::map<std::uint64_t,std::wstring> labels;
    bool show_grid = true;
    epidemic::game::editor::GizmoTool tool = epidemic::game::editor::GizmoTool::Move;
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
                view.scene.reset();
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
                if (view.scene)
                    view.scene->Pump(*view.renderer);
                view.renderer->Begin();
                // Same native editor surface integration: remove gameplay letterboxing.
                view.renderer->PushTransform(
                    D2D1::Matrix3x2F::Scale(1 / base, 1 / base) *
                    D2D1::Matrix3x2F::Translation(-offset_x / base, -offset_y / base));
                view.renderer->Rect(0, 0, size.x, size.y, 0x191920);
                view.camera.Resize(size.x, size.y);
                const auto origin = view.editing_scene ? view.camera.ToScreen({}) :
                    epidemic::game::editor::ViewPoint{};
                const float x0 = static_cast<float>(std::fmod(origin.x,48.0));
                const float y0 = static_cast<float>(std::fmod(origin.y,48.0));
                if (view.show_grid) {
                    for (float x = x0; x < size.x; x += 48)
                        view.renderer->Line(x, 0, x, size.y, 0x363640);
                    for (float y = y0; y < size.y; y += 48)
                        view.renderer->Line(0, y, size.x, y, 0x363640);
                }
                if (view.editing_scene) {
                    if (origin.x >= 0 && origin.x < size.x)
                        view.renderer->Line(static_cast<float>(origin.x),0,static_cast<float>(origin.x),size.y,0x505f76);
                    if (origin.y >= 0 && origin.y < size.y)
                        view.renderer->Line(0,static_cast<float>(origin.y),size.x,static_cast<float>(origin.y),0x744e54);
                }
                if (view.scene)
                    view.scene->Draw(*view.renderer, D2D1::SizeF(size.x, size.y), view.editing_scene ? &view.camera : nullptr);
                else if (!view.editing_scene)
                    view.renderer->RoundRect(size.x / 2 - 50, size.y / 2 - 36, 100, 72, 6,
                                             0xa855d5);
                for (const auto &item : view.camera.Items()) {
                    const auto b = view.camera.ScreenBounds(item);
                    if (b.left >= size.x || b.top >= size.y || b.right <= 0 || b.bottom <= 0)
                        continue;
                    const float l = static_cast<float>(std::clamp(b.left, -1.0, static_cast<double>(size.x) + 1.0));
                    const float r = static_cast<float>(std::clamp(b.right, -1.0, static_cast<double>(size.x) + 1.0));
                    const float t = static_cast<float>(std::clamp(b.top, -1.0, static_cast<double>(size.y) + 1.0));
                    const float d = static_cast<float>(std::clamp(b.bottom, -1.0, static_cast<double>(size.y) + 1.0));
                    if(const auto label=view.labels.find(item.id.Raw());label!=view.labels.end()){
                        view.renderer->Rect(l,t,r-l,d-t,0x273746);view.renderer->Text(label->second,l,std::max(0.0f,t-22),14,0xf0f0f4,std::max(200.0f,r-l));
                    }
                    const auto color = item.id == view.selected ? 0x78bbffu : 0x687787u;
                    view.renderer->Line(l,t,r,t,color);
                    view.renderer->Line(r,t,r,d,color);
                    view.renderer->Line(r,d,l,d,color);
                    view.renderer->Line(l,d,l,t,color);
                    if (item.id == view.selected) {
                        namespace ed = epidemic::game::editor;
                        const auto gizmo = ed::MakeGizmo(view.camera,item.world,ed::GizmoSpace::Local);
                        const float x = static_cast<float>(gizmo.pivot.x), y = static_cast<float>(gizmo.pivot.y);
                        if (view.tool == ed::GizmoTool::Rotate)
                            view.renderer->Outline(x,y,52,0x78bbffu);
                        else {
                            view.renderer->Line(x,y,static_cast<float>(gizmo.x.x),static_cast<float>(gizmo.x.y),0xe36d6du,3);
                            view.renderer->Line(x,y,static_cast<float>(gizmo.y.x),static_cast<float>(gizmo.y.y),0x79cf8au,3);
                            view.renderer->Rect(x-5,y-5,10,10,0x78bbffu);
                        }
                    }
                }
                view.renderer->Text(view.editing_scene ? (view.camera.Items().empty() ? L"Empty Scene" : L"")
                                                       : view.scene ? L"" : L"U1 / existing D3D11 + D2D Renderer",
                                    20, 20, 16, 0xf0f0f4, size.x - 40);
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
        // Diagnostics do not create an application model or callbacks into Java.
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
        std::size_t preview_bytes = 0, preview_textures = 0, preview_invariants = 0;
        for (const auto &[id, view] : Views())
            if (view.scene) {
                (void)id;
                const auto stats = view.scene->Memory();
                preview_bytes += stats.resident_bytes;
                preview_textures += view.scene->TextureCount();
                preview_invariants += stats.invariant_failure_count;
            }
        const jlong values[]{static_cast<jlong>(Views().size()),
                             static_cast<jlong>(handles),
                             static_cast<jlong>(
                                 GetGuiResources(GetCurrentProcess(), GR_GDIOBJECTS)),
                             static_cast<jlong>(memory.WorkingSetSize),
                             static_cast<jlong>(memory.PrivateUsage),
                             static_cast<jlong>(threads),
                             static_cast<jlong>(preview_bytes),
                             static_cast<jlong>(preview_textures),
                             static_cast<jlong>(preview_invariants)};
        auto result = env->NewLongArray(9);
        if (result)
            env->SetLongArrayRegion(result, 0, 9, values);
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

extern "C" JNIEXPORT jstring JNICALL Java_epidemic_u1_NativeViewport_scene(
    JNIEnv *env, jclass, jlong id, jstring root, jobjectArray keys, jfloatArray geometry) noexcept {
    try {
        CheckThread();
        auto found = Views().find(id);
        if (found == Views().end())
            throw std::runtime_error("Stale viewport handle");
        const auto count = env->GetArrayLength(keys);
        if (count < 0 || count > 262144 || env->GetArrayLength(geometry) != count * 8)
            throw std::runtime_error("Preview geometry bounds");
        const auto chars = env->GetStringChars(root, nullptr);
        if (!chars)
            throw std::runtime_error("Preview path unavailable");
        std::wstring value(reinterpret_cast<const wchar_t *>(chars),
                           static_cast<std::size_t>(env->GetStringLength(root)));
        env->ReleaseStringChars(root, chars);
        const std::filesystem::path path{value};
        std::vector<float> coordinates(static_cast<std::size_t>(count) * 8);
        env->GetFloatArrayRegion(geometry, 0, count * 8, coordinates.data());
        if (env->ExceptionCheck())
            throw std::runtime_error("Preview geometry unavailable");
        std::vector<epidemic::editor_host::PreviewSprite> sprites;
        for (jsize i = 0; i < count; ++i) {
            auto key = static_cast<jstring>(env->GetObjectArrayElement(keys, i));
            const auto text = env->GetStringUTFChars(key, nullptr);
            if (!text)
                throw std::runtime_error("Preview key unavailable");
            std::string digest(text);
            env->ReleaseStringUTFChars(key, text);
            env->DeleteLocalRef(key);
            if (digest.size() != 64 ||
                digest.find_first_not_of("0123456789abcdef") != std::string::npos)
                throw std::runtime_error("Preview digest invalid");
            epidemic::editor_host::PreviewSprite sprite;
            sprite.key = std::move(digest);
            for (std::size_t n = 0; n < 8; ++n) {
                sprite.geometry[n] = coordinates[static_cast<std::size_t>(i) * 8 + n];
                if (!std::isfinite(sprite.geometry[n]))
                    throw std::runtime_error("Preview finite geometry required");
            }
            if (sprite.geometry[5] <= 0)
                throw std::runtime_error("Preview pixels per unit invalid");
            sprites.push_back(std::move(sprite));
        }
        auto &view = found->second;
        if (!view.scene || view.scene->Root() != path)
            view.scene = std::make_unique<epidemic::editor_host::ScenePreview>(path);
        view.scene->Update(std::move(sprites));
        return Reply(env, "PASS scene projection");
    } catch (const std::exception &error) {
        return Failure(env, error.what());
    } catch (...) {
        return Failure(env, "unknown scene preview failure");
    }
}

extern "C" JNIEXPORT jboolean JNICALL Java_epidemic_u1_NativeViewport_sceneItems(
    JNIEnv *env, jclass, jlong surface, jlong document, jlong selection, jlongArray ids,
    jdoubleArray bounds,jobjectArray labels) noexcept {
    try {
        CheckThread();
        auto &view = Views().at(surface);
        const auto size = env->GetArrayLength(ids);
        if (size < 0 || size > 4096 || env->GetArrayLength(bounds) != size * 7)
            return JNI_FALSE;
        std::vector<jlong> keys(static_cast<std::size_t>(size));
        std::vector<jdouble> values(keys.size() * 7);
        env->GetLongArrayRegion(ids, 0, size, keys.data());
        env->GetDoubleArrayRegion(bounds, 0, size * 7, values.data());
        if (env->ExceptionCheck())
            return JNI_FALSE;
        namespace ed = epidemic::game::editor;
        namespace rt = epidemic::runtime;
        std::vector<ed::SceneViewItem> items;
        const auto label_count=labels?env->GetArrayLength(labels):0;if(label_count!=0&&label_count!=size)return JNI_FALSE;
        view.labels.clear();
        items.reserve(keys.size());
        for (std::size_t index = 0; index < keys.size(); ++index) {
            if(label_count){auto label=static_cast<jstring>(env->GetObjectArrayElement(labels,static_cast<jsize>(index)));if(!label)return JNI_FALSE;const auto length=env->GetStringLength(label);if(length>1024){env->DeleteLocalRef(label);return JNI_FALSE;}const auto *text=env->GetStringChars(label,nullptr);if(!text){env->DeleteLocalRef(label);return JNI_FALSE;}view.labels.emplace(static_cast<std::uint64_t>(keys[index]),std::wstring(reinterpret_cast<const wchar_t*>(text),static_cast<std::size_t>(length)));env->ReleaseStringChars(label,text);env->DeleteLocalRef(label);}
            const auto *value = values.data() + index * 7;
            if (!std::all_of(value, value + 7, [](double v) { return std::isfinite(v); }) ||
                value[0] > value[2] || value[1] > value[3] ||
                std::abs(value[4]) > std::numeric_limits<float>::max() ||
                std::abs(value[5]) > std::numeric_limits<float>::max())
                return JNI_FALSE;
            rt::Transform world{};
            world.position.x = static_cast<float>(value[4]);
            world.position.y = static_cast<float>(value[5]);
            world.rotation.z = static_cast<float>(std::sin(value[6] * .5));
            world.rotation.w = static_cast<float>(std::cos(value[6] * .5));
            items.push_back({rt::SceneNodeId{static_cast<std::uint64_t>(keys[index])}, {},
                             {value[0], value[1], value[2], value[3]}, world});
        }
        view.editing_scene = document != 0;
        view.selected = rt::SceneNodeId{static_cast<std::uint64_t>(selection)};
        if (view.editing_scene) {
            const auto scene = rt::AssetId{epidemic::foundation::StringId{static_cast<std::uint64_t>(document)}};
            const bool initial_fit = view.camera.Document() != scene || (view.camera.Items().empty() && !items.empty());
            view.camera.Project(scene,std::move(items));
            if (initial_fit && !view.camera.Items().empty()) {
                const auto extent = view.camera.Extent();
                view.camera.Zoom({extent.x*.5,extent.y*.5},-1);
            }
        } else
            view.camera.Clear();
        return JNI_TRUE;
    } catch (...) {
        return JNI_FALSE;
    }
}

extern "C" JNIEXPORT jlong JNICALL Java_epidemic_u1_NativeViewport_sceneInput(
    JNIEnv *, jclass, jlong surface, jint operation, jdouble x, jdouble y, jdouble delta) noexcept {
    try {
        CheckThread();
        auto &view = Views().at(surface);
        if (!view.editing_scene || !std::isfinite(x) || !std::isfinite(y) || !std::isfinite(delta))
            return 0;
        const auto size = view.camera.Extent();
        const epidemic::game::editor::ViewPoint point{x * size.x, y * size.y};
        switch (operation) {
        case 0: return static_cast<jlong>(view.camera.Pick(point).Raw());
        case 1: view.camera.Pan(point); break;
        case 2: view.camera.Zoom(point, delta); break;
        case 3: view.camera.Fit(); break;
        case 4:
            if (delta >= 0 && delta <= 2)
                view.tool = static_cast<epidemic::game::editor::GizmoTool>(static_cast<int>(delta));
            break;
        case 5: view.show_grid = delta != 0; break;
        }
    } catch (...) {
        return 0;
    }
    return 0;
}

extern "C" JNIEXPORT jdoubleArray JNICALL Java_epidemic_u1_NativeViewport_scenePointer(
    JNIEnv *env, jclass, jlong surface, jdouble x, jdouble y) noexcept {
    try {
        CheckThread();
        const auto &view = Views().at(surface);
        if (!view.editing_scene || !std::isfinite(x) || !std::isfinite(y))
            return env->NewDoubleArray(0);
        namespace ed = epidemic::game::editor;
        const auto extent = view.camera.Extent();
        const ed::ViewPoint cursor{x * extent.x,y * extent.y};
        const auto center = view.camera.ToWorld({extent.x*.5,extent.y*.5});
        const auto world = view.camera.ToWorld(cursor);
        auto handle = ed::GizmoHandle::None;
        const auto item = std::ranges::find(view.camera.Items(),view.selected,&ed::SceneViewItem::id);
        if (item != view.camera.Items().end())
            handle = ed::PickGizmo(ed::MakeGizmo(view.camera,item->world,ed::GizmoSpace::Local),view.tool,cursor);
        const jdouble data[]{cursor.x,cursor.y,extent.x,extent.y,center.x,center.y,view.camera.Scale(),
                             static_cast<double>(handle),world.x,world.y};
        auto result = env->NewDoubleArray(static_cast<jsize>(std::size(data)));
        if (result)
            env->SetDoubleArrayRegion(result,0,static_cast<jsize>(std::size(data)),data);
        return result;
    } catch (...) {
        return env->NewDoubleArray(0);
    }
}
