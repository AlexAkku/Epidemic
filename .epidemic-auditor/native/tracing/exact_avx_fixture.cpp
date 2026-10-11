#include <immintrin.h>
#include <intrin.h>
#include <cstdio>

extern "C" __declspec(noinline) __m256 __vectorcall AuditorExactWideRegisters(__m256 left, __m256 right) {
    return _mm256_add_ps(left, right);
}
int main() {
    int cpu[4]{};
    __cpuid(cpu, 1);
    if ((cpu[2] & (1 << 27)) == 0 || (_xgetbv(0) & 6) != 6)
        return 77;
    __cpuidex(cpu, 7, 0);
    if ((cpu[1] & (1 << 5)) == 0)
        return 77;
    const auto left = _mm256_setr_ps(1, 2, 3, 4, 5, 6, 7, 8);
    const auto right = _mm256_setr_ps(8, 7, 6, 5, 4, 3, 2, 1);
    alignas(32) float values[8]{};
    _mm256_store_ps(values, AuditorExactWideRegisters(left, right));
    for (float value : values)
        if (value != 9)
            return 2;
    std::puts("actual AVX2 vectorcall arguments/results preserved");
    return 0;
}
