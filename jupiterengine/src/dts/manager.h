#pragma once 

#include "datastructure/buffer.h"
#include "dts/messageQueue.h"
#include "dts/treeState.h"
#include "dts/types.h"
#include "search/searcher.h"
#include <condition_variable>
#include <cstddef>

const std::size_t DTS_MAX_THREADS = 8;

// TODO: Give each worker access to the searcher, etc
//          - Maybe just pass it in?

// TODO: Move the non-shared stuff out of the searcher and into the workers

// TODO: How will the workers all use the shared resources on the searcher?

class DTSManager {
public:
    DTSManager(const char *fen);
    ~DTSManager();

    Move FindBest(uint64_t msLeft);
    void MakeMove(Move move);
    BoardState State() { return m_State; }
    
    // Configuration
    void SetTimeControl(float seconds, float increment);
    void SetWeights(const EvaluatorConstants& weights);

    // Inter-thread communication
    bool HasMessage(ThreadId me);
    Message ReadMessage(ThreadId me);
    void SendMessage(Command::Value command, ThreadId me);
    void SendMessageTo(Command::Value command, ThreadId me, ThreadId to);

private:
    Zobrist m_Zobrist{};
    PieceSquareTables m_PieceSquareTables{};
    Searcher m_Searcher{m_Zobrist, m_PieceSquareTables};
    BoardState m_State{};

    std::condition_variable m_Condition{};
    bool m_Stopping{false};

    Buffer<std::thread, DTS_MAX_THREADS> m_Threads;
    Buffer<MessageQueue *, DTS_MAX_THREADS> m_Queues;   // Each thread's message queue for comms
    Buffer<TreeState, DTS_MAX_THREADS> m_TreeStates;    // Each thread's tree state block that they will write to during help
    uint8_t *m_Arena{nullptr};                          // Memory arena for tree nodes and message queues
};
