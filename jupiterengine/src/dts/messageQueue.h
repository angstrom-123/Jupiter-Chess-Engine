#pragma once 

#include "datastructure/circularQueue.h"
#include "dts/treeState.h"
#include "dts/types.h"
#include "evaluation/evaluator.h"
#include <cstdint>

struct Command {
    typedef enum : uint8_t {
        NONE,
        STATE,
        TUNE,
        HELP,
    } Value;
};

// Good old discriminated union
struct Message {
    Command::Value command{Command::NONE};
    ThreadId sentBy{THREAD_ID_MAX};
    
    union {
        BoardState *state;              // STATE
        EvaluatorConstants *weights;    // TUNE
        TreeState *helpBlock;           // HELP
    };
};

// Avoid false sharing
class alignas(64) MessageQueue {
public:
    bool Empty();
    void Enqueue(const Message& message);
    Message Dequeue(); // Copies out of the queue to minimise time locked

private:
    CircularQueue<Message, 16> m_Queue;
    std::mutex m_Lock;
};
