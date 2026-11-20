#pragma once

#ifdef _MSC_VER 
    #define INLINE __forceinline
#elif defined(__GNUC__) || defined(__clang__)
    #define INLINE inline __attribute__((always_inline))
#else 
    #define INLINE inline
#endif
