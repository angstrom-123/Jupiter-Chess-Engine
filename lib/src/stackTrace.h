#pragma once 

#include <cstdint>
#include "circularStack.h"
#include "log.h"

struct StackFrame {
    const char *functionName{nullptr};
    const char *fileName{nullptr};
    int64_t lineNumber{INT64_MAX};
};

using CallStack = CircularStack<StackFrame, 20>;

class StackTracer {
public:
    static CallStack& GetFrames() 
    {
        thread_local CallStack frames = CallStack();
        return frames;
    }

    static void PrintTrace() 
    {
        WARN("\n\n=== C++ Stack Trace ===\n");

        CallStack& frames = GetFrames();
        if (frames.Size() == 0) {
            WARN("  [EMPTY]");
            return;
        }

        std::size_t count = frames.Size();
        for (std::size_t i = 0; i < count; i++) {
            const StackFrame& frame = frames.Top();
            _LOG(loglevel::INFO, "  [" << i << "] ");
            _LOG(loglevel::WARN, frame.functionName << "() ");
            INFO("at " << frame.fileName << ":" << frame.lineNumber);
            frames.Pop();
        }

        WARN("\n=======================\n");
    }
};

class StackInstrumenter {
public:
    StackInstrumenter(const char *functionName, const char *fileName, int64_t lineNumber) 
    {
        StackTracer::GetFrames().Push({ functionName, fileName, lineNumber });
    }

    ~StackInstrumenter()
    {
        StackTracer::GetFrames().Pop();
    }
};
