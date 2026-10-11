#include <cstdio>
#include <cstring>
#include <thread>
#include <stdexcept>
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
extern "C" __declspec(dllimport) int AuditorExactDllCall(int value);
extern "C" void _penter();
extern "C" void _pexit();

extern "C" __declspec(noinline) int AuditorExactLeaf(int value) {
    return value * 3 + 1;
}
extern "C" __declspec(noinline) int AuditorExactRecursive(int depth) {
    return depth ? AuditorExactRecursive(depth - 1) + AuditorExactLeaf(depth) : 1;
}
extern "C" __declspec(noinline) double AuditorExactRegisters(double a, double b, double c, double d, int e, int f,
                                                             int g, int h) {
    return a + b + c + d + e + f + g + h;
}
__declspec(noinline) void AuditorExactThrow() {
    throw std::runtime_error("controlled unwind");
}
extern "C" __declspec(noinline) void AuditorExactWorker() {
    if (AuditorExactRecursive(3) != 22)
        std::terminate();
}
extern "C" __declspec(noinline) int AuditorExactHotPath(bool regression) {
    // Known detector control: equal logical output, physically different latency.
    Sleep(regression ? 40UL : 1UL);
    return AuditorExactLeaf(2);
}
int main(int argc, char **argv) {
    if (argc > 1 && (std::strcmp(argv[1], "hot-baseline") == 0 || std::strcmp(argv[1], "hot-regression") == 0)) {
        const bool regression = std::strcmp(argv[1], "hot-regression") == 0;
        for (unsigned index = 0; index < 5; ++index)
            if (AuditorExactHotPath(regression) != 7)
                return 5;
        return 0;
    }
    if (argc > 1 && std::strcmp(argv[1], "unwind-check") == 0) {
        DWORD64 base = 0;
        if (!RtlLookupFunctionEntry(reinterpret_cast<DWORD64>(&_penter), &base, nullptr) ||
            !RtlLookupFunctionEntry(reinterpret_cast<DWORD64>(&_pexit), &base, nullptr))
            return 4;
    }
    if (AuditorExactRegisters(1.25, 2.5, 3.75, 4.0, 5, 6, 7, 8) != 37.5)
        return 2;
    if (argc > 1 && std::strcmp(argv[1], "cross-dll") == 0 && AuditorExactDllCall(2) != 13)
        return 3;
    if (argc > 1 && std::strcmp(argv[1], "unbalanced") == 0) {
        try {
            AuditorExactThrow();
        } catch (const std::runtime_error &) {
            // The expected exception leaves missing /GH exits for the analyzer to detect.
            std::puts("controlled unwind observed");
        }
    } else {
        AuditorExactWorker();
        std::thread worker(AuditorExactWorker);
        worker.join();
    }
    std::puts("exact native fixture: actual ABI/result checks passed");
    return 0;
}
