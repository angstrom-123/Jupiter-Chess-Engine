#pragma once 

#include <cstdint>
#include <vector>
#include <iostream>
#include <filesystem>

namespace fs = std::filesystem;

#define JUPITER_TRACE() StackInstrumenter _trace(__func__, __FILE__, __LINE__)

struct StackFrame {
    const char *functionName{nullptr};
    const char *fileName{nullptr};
    int64_t lineNumber{INT64_MAX};
};

class StackTracer {
public:
    static std::vector<StackFrame>& GetFrames() 
    {
        thread_local std::vector<StackFrame> frames;
        frames.reserve(512);
        return frames;
    }

    static void PrintTrace(std::size_t maxDepth = 20) 
    {
        std::cerr << "\n\n==== C++ Stack Trace ====\n\n";

        const std::vector<StackFrame>& frames = GetFrames();
        if (frames.empty()) {
            std::cout << "Stack Trace Empty" << std::endl;
            return;
        }

        for (std::size_t i = std::min(maxDepth, frames.size()); i > 0; i--) {
            const StackFrame& frame = frames[i - 1];
            std::cout << frames.size() - i << ". " << frame.functionName << "() at " << frame.fileName << ":" << frame.lineNumber << std::endl;
        }

        std::cerr << "\n=========================\n\n";
    }
};

class StackInstrumenter {
public:
    StackInstrumenter(const char *functionName, const char *fileName, int64_t lineNumber) 
    {
        StackTracer::GetFrames().push_back({ functionName, fileName, lineNumber });
    }

    ~StackInstrumenter()
    {
        StackTracer::GetFrames().pop_back();
    }
};
