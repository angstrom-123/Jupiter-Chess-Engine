#include "inlines.h"

#if defined(__x86_64__) || defined(_M_X64) || defined(__i386__) || defined(_M_IX86)
    #include <immintrin.h>
    INLINE void CpuPause() noexcept { _mm_pause(); }
#elif defined(__aarch64) || defined(_M_ARM64)
    INLINE void CpuPause() noexcept { asm volatile("yield" ::: "memory"); }
#else 
    #warning No CPU Pause instruction on this platform
    INLINE void CpuPause() noexcept {}
#endif
