#define WIN32_LEAN_AND_MEAN
#include <windows.h>

// An unloadable module lets the existing Windows Leak Verifier observe a
// resource surviving its allocating module. It is never linked into the Engine.
extern "C" __declspec(dllexport) int AuditorLeakFixture(int leak) {
    auto *memory = HeapAlloc(GetProcessHeap(), 0, 128);
    if (!memory)
        return 1;
    static_cast<char *>(memory)[0] = 42;
    if (!leak && !HeapFree(GetProcessHeap(), 0, memory))
        return 2;
    return 0;
}
