#pragma once

#ifndef NOTRACE
    #include "stackTrace.h"
    #define JUPITER_TRACE() StackInstrumenter _trace(__func__, __FILE__, __LINE__)
#else
    #define JUPITER_TRACE()
#endif
