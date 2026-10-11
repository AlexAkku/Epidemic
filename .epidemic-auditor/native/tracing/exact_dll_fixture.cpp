extern "C" __declspec(noinline) int AuditorExactDllLeaf(int value) {
    return value + 4;
}
extern "C" __declspec(dllexport) __declspec(noinline) int AuditorExactDllCall(int value) {
    return AuditorExactDllLeaf(value) + AuditorExactDllLeaf(value + 1);
}
