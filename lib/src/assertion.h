#pragma once 

#ifdef DEBUG 
    #include "exception.h"
    #define ASSERT(expr) if (!(expr)) throw JupiterException("Assertion failure: '" QUOTE(expr) "'")
#else 
    #define ASSERT(expr)
#endif
