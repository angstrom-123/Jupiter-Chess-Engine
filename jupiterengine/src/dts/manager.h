#pragma once 

#include "datastructure/buffer.h"
#include "dts/messageQueue.h"
#include "dts/treeState.h"
#include "dts/types.h"
#include <atomic>
#include <condition_variable>
#include <cstddef>
#include <thread>

// const std::size_t DTS_MAX_THREADS = 8;
// TODO: Increase this again
const std::size_t DTS_MAX_THREADS = 1;

class DTSManager {
public:
    DTSManager();
    ~DTSManager();

    // External API
    Move FindBest(uint64_t msLeft);
    void TelemetryJSON(std::string& result) const;
    void MetricsJSON(std::string& result) const;

    // Inter-thread communication
    Message WorkerReadMessage(ThreadId me);
    bool WorkerShouldStop(ThreadId me);
    void WorkerSetSearchComplete();
    void WorkerUpdateBestMove(Move move);
    void WorkerSendMessage(Command::Value command, ThreadId me);
    void WorkerSendMessageTo(Command::Value command, ThreadId me, ThreadId to);

public:
    // For initialising threads and interfacing with the board
    BoardState state{};
    float tcSeconds{0};
    float tcIncrement{0};
    EvaluatorConstants weights{EvaluatorConstants::Tuned()};

private:
    void SendMessage(Message message);

private:
    // The search done boolean is set to true by the workers if they have finished processing. That 
    // is protected by this mutex. The main thread waits on this condition variable until either the 
    // search is done or the maximum search time is reached. In case of timeout (where search done 
    // is false), a boolean flag is set telling all workers to terminate.
    bool m_SearchDone;
    std::mutex m_SearchDoneMutex;
    std::condition_variable m_SearchDoneCondition;

    std::atomic<bool> m_StopRequested;  // For telling the workers to abort
    std::atomic<uint32_t> m_BestMove;  

    Buffer<std::thread, DTS_MAX_THREADS> m_Threads;
    Buffer<MessageQueue *, DTS_MAX_THREADS> m_Queues;   // Each thread's message queue for comms
    Buffer<TreeState, DTS_MAX_THREADS> m_TreeStates;    // Each thread's tree state block that they will write to during help
    uint8_t *m_Arena{nullptr};                          // Memory arena for tree nodes and message queues
};
