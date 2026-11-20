#pragma once 

#include <atomic>
#include <cmath>
#include <cstdint>
#include "datastructure/circularStack.h"
#include "util/log.h"

struct StackFrame {
    const char *functionName{nullptr};
    const char *fileName{nullptr};
    int64_t lineNumber{INT64_MAX};
};

constexpr std::size_t MAX_STACK_FRAMES = 20;

struct CallStack {
    CircularStack<StackFrame, MAX_STACK_FRAMES> frames;
    std::size_t index{SIZE_MAX};

    CallStack();
    ~CallStack();
};

constexpr std::size_t MAX_THREADS = 128;
static std::atomic<CallStack *> g_CallStackRegistry[MAX_THREADS]{};

INLINE CallStack::CallStack()
{
    for (std::size_t i = 0; i < MAX_THREADS; i++) {
        CallStack *stack = nullptr;
        if (g_CallStackRegistry[i].compare_exchange_strong(stack, this)) {
            index = i;
            break;
        }
    }
}

INLINE CallStack::~CallStack()
{
    if (index != SIZE_MAX)
        g_CallStackRegistry[index].store(nullptr, std::memory_order_release);
}

class StackTracer {
public:
    static CallStack& GetStack() 
    {
        thread_local CallStack stack = CallStack();
        return stack;
    }

    static void PrintTrace() 
    {
        WARN("\n\n=== C++ Stack Trace ===\n");

        CallStack& stack = GetStack();
        if (stack.frames.Size() == 0) {
            WARN("  [EMPTY]");
            return;
        }

        std::size_t count = std::min(MAX_STACK_FRAMES, stack.frames.Size());
        for (std::size_t i = 0; i < count; i++) {
            const StackFrame& frame = stack.frames.Top();
            _LOG(loglevel::INFO, "  [" << i << "] ");
            _LOG(loglevel::WARN, frame.functionName << "() ");
            INFO("at " << frame.fileName << ":" << frame.lineNumber);
            stack.frames.Pop();
        }

        WARN("\n=======================\n");
    }

    static void PrintAllTraces()
    {
        for (std::size_t i = 0; i < MAX_THREADS; i++) {
            WARN("\n\n=== C++ Stack Trace (thread " << i << ") ===\n");

            CallStack *stack = g_CallStackRegistry[i].load(std::memory_order_acquire);
            if (stack == nullptr)
                continue;

            if (stack->frames.Size() == 0) {
                WARN("  [EMPTY]");
                return;
            }

            std::size_t count = std::min(MAX_STACK_FRAMES, stack->frames.Size());
            for (std::size_t i = 0; i < count; i++) {
                const StackFrame& frame = stack->frames.Top();
                _LOG(loglevel::INFO, "  [" << i << "] ");
                _LOG(loglevel::WARN, frame.functionName << "() ");
                INFO("at " << frame.fileName << ":" << frame.lineNumber);
                stack->frames.Pop();
            }

            WARN("\n==================================\n");
        }
    }
};

class StackInstrumenter {
public:
    StackInstrumenter(const char *functionName, const char *fileName, int64_t lineNumber) 
    {
        StackTracer::GetStack().frames.Push({ functionName, fileName, lineNumber });
    }

    ~StackInstrumenter()
    {
        StackTracer::GetStack().frames.Pop();
    }
};
