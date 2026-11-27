#include "manager.h"
#include "dts/types.h"
#include "util/instrumenter.h"
#include "worker.h"
#include <atomic>

DTSManager::DTSManager()
{
    JUPITER_TRACE();

    // Allocate everything
    constexpr std::size_t queueBytesOffset = 0;
    constexpr std::size_t queueBytesPerThread = sizeof(MessageQueue); // Each struct is already aligned to 64

    constexpr std::size_t nodeBytesOffset = DTS_MAX_THREADS * queueBytesPerThread;
    constexpr std::size_t nodeBytesPerThread = (MAX_PLY * sizeof(TreeNode) / 64) * 64; // Align each allocation to 64

    constexpr std::size_t arenaBytes = (queueBytesPerThread + nodeBytesPerThread) * DTS_MAX_THREADS;
    m_Arena = new uint8_t[arenaBytes];

    // Suballocate each worker's share of shared memory
    for (ThreadId i = 0; i < DTS_MAX_THREADS; i++) {
        m_Queues.PushBack(reinterpret_cast<MessageQueue *>(&m_Arena[queueBytesOffset + queueBytesPerThread * i]));
        m_TreeStates.PushBack(TreeState {
            .nodes = reinterpret_cast<TreeNode *>(&m_Arena[nodeBytesOffset + nodeBytesPerThread * i]),
            .count = 0
        });
    }
}

DTSManager::~DTSManager()
{
    JUPITER_TRACE();

    delete[] m_Arena;
}

Move DTSManager::FindBest(uint64_t msLeft)
{
    JUPITER_TRACE();

    m_SearchDone = false;
    m_StopRequested.store(false, std::memory_order_relaxed);
    m_BestMove.store(0, std::memory_order_relaxed);

    // TODO: OPTIMISATION:
    //       Might be better to have a live threadpool the whole time and just send it commands to 
    //       initialise each thread for each search, rather than spawning new threads each time.
    for (ThreadId i = 0; i < DTS_MAX_THREADS; i++) {
        m_Threads.EmplaceBack([this, i] { Worker::EntryPoint(*this, i); });
    }

    // Tell one thread to kick off the dts search, it will bring in the others 
    Message message {
        .command = Command::GO,
        .sentBy = MANAGER_THREAD_ID,
        .msLeft = msLeft
    };
    m_Queues[0]->Enqueue(message);

    INFO("Manager just enqueued the go command");
    {
        std::unique_lock<std::mutex> lock(m_SearchDoneMutex);

        // Wait until we finish searching or time out
        m_SearchDoneCondition.wait_for(lock, std::chrono::milliseconds(msLeft), [this] { return m_SearchDone; });
    }

    INFO("Manager is out of the wait");

    m_StopRequested.store(true, std::memory_order_release);
    for (ThreadId i = 0; i < DTS_MAX_THREADS; i++) {
        m_Threads[i].join();
    }
    m_Threads.Clear();

    INFO("Manager joined all threads");

    // At this point, workers will have told us the best move
    uint32_t bestMove = m_BestMove.load(std::memory_order_acquire);
    return (bestMove > 0) ? Move::Unpack(bestMove) : Move::Invalid();
}

void DTSManager::TelemetryJSON(std::string& result) const
{
    JUPITER_TRACE();

    // TODO
    //  depth
    //  nodesSearched
    //  nodesLookedUp
    //  nodesQuiesced
    //  pawnsLookedUp
    //  evaluations
    //  searchTime
    result = "Not implemented: telemetry";
}

void DTSManager::MetricsJSON(std::string& result) const
{
    JUPITER_TRACE();

    // TODO:
    //  ttSize
    //  ptSize
    //  bookMoves
    result = "Not implemented: metrics";
}

Message DTSManager::WorkerReadMessage(ThreadId me)
{
    JUPITER_TRACE();

    return m_Queues[me]->TryDequeue();
}

bool DTSManager::WorkerShouldStop(ThreadId me)
{
    JUPITER_TRACE();

    (void) me;
    return m_StopRequested.load(std::memory_order_acquire);
}

void DTSManager::WorkerSetSearchComplete()
{
    JUPITER_TRACE();

    INFO("Worker set the search as complete");

    // Safely set the flag
    m_SearchDoneMutex.lock();
    m_SearchDone = true;
    m_SearchDoneMutex.unlock();

    // Notify the manager that there has been a change
    m_SearchDoneCondition.notify_one();
}

void DTSManager::WorkerUpdateBestMove(Move move)
{
    JUPITER_TRACE();
    INFO("Worker stored the best move: " << move.ToLAN().chars);

    m_BestMove.store(move.Pack(), std::memory_order_release);
}

void DTSManager::WorkerSendMessage(Command::Value command, ThreadId me)
{
    JUPITER_TRACE();

    for (ThreadId i = 0; i < DTS_MAX_THREADS; i++) {
        if (i == me) continue;
        
        WorkerSendMessageTo(command, me, i);
    }
}

void DTSManager::WorkerSendMessageTo(Command::Value command, ThreadId me, ThreadId to)
{
    JUPITER_TRACE();

    if (!Command::IsWorkerCommand(command))
        ERROR("Worker is sending a manager-only command: " << Command::Show(command));

    switch (command) {
        case Command::WORKER_HELP: // Ask the receiver to copy their tree to the provided memory
        {
            Message message {
                .command = command,
                .sentBy = me,
                .helpBlock = &m_TreeStates[to]
            };
            m_Queues[to]->Enqueue(message);
            break;
        }
        default:
            ERROR("Command not implemented: " << Command::Show(command));
            std::unreachable();
    }
}
