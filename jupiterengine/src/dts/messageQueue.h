#pragma once 

#include "datastructure/circularQueue.h"
#include "dts/treeState.h"
#include "dts/types.h"
#include "evaluation/evaluator.h"
#include <cstdint>

struct Command {
    typedef enum : uint8_t {
        NONE,
        // Only the manager can send these
        TUNE,
        GO,
        // Only the workers can send these
        WORKER_HELP,
    } Value;

    INLINE static constexpr const char *Show(Value command)
    {
        switch (command) {
            case NONE: return "None";
            case TUNE: return "Tune";
            case GO: return "Go";
            case WORKER_HELP: return "Worker Help";
        };
    }
    INLINE static constexpr bool IsManagerCommand(Value command)
    {
        // TODO: Update this as more commands are added
        return command == Command::TUNE || command == Command::GO;
    }
    INLINE static constexpr bool IsWorkerCommand(Value command)
    {
        return !IsManagerCommand(command);
    }
};

// Good old discriminated union
struct Message {
    Command::Value command{Command::NONE};
    ThreadId sentBy{THREAD_ID_MAX};
    
    union {
        EvaluatorConstants *weights;    // TUNE
        uint64_t msLeft;                // GO
        TreeState *helpBlock;           // HELP
    };
};

// Avoid false sharing
class alignas(64) MessageQueue {
public:
    MessageQueue() noexcept = default;
    void Enqueue(const Message& message);
    Message TryDequeue(); // Copies out of the queue to minimise time locked

private:
    CircularQueue<Message, 16> m_Queue{};
    std::mutex m_Lock{};
};
