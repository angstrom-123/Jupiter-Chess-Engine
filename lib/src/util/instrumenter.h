#pragma once

#ifndef NOTRACE
    #include "stackTrace.h"
    #define JUPITER_TRACE() StackInstrumenter _trace(__func__, __FILE__, __LINE__)
#else
    #define JUPITER_TRACE()
#endif

#ifdef PROFILE
    #include "profiler.h"
    #define JUPITER_PROFILING_END() Profiler::EndSession()
    #define JUPITER_PROFILE() ProfileInstrumenter _profile(__func__, __FILE__, __LINE__)
#else 
    #define JUPITER_PROFILING_END()
    #define JUPITER_PROFILE()
#endif
