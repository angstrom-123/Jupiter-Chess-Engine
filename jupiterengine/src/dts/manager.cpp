#include "manager.h"
#include "util/fenParser.h"
#include "worker.h"

DTSManager::DTSManager(const char *fen)
{
    // Allocate everything
    constexpr std::size_t queueBytesOffset = 0;
    constexpr std::size_t queueBytesPerThread = sizeof(MessageQueue); // Each struct is already aligned to 64

    constexpr std::size_t nodeBytesOffset = DTS_MAX_THREADS * queueBytesPerThread;
    constexpr std::size_t nodeBytesPerThread = (MAX_PLY * sizeof(TreeNode) / 64) * 64; // Align each allocation to 64

    constexpr std::size_t arenaBytes = (queueBytesPerThread + nodeBytesPerThread) * DTS_MAX_THREADS;
    m_Arena = new uint8_t[arenaBytes];

    // Suballocate the worker's share of shared memory and spawn the thread 
    for (ThreadId i = 0; i < DTS_MAX_THREADS; i++) {
        m_Queues[i] = reinterpret_cast<MessageQueue *>(m_Arena[queueBytesOffset + queueBytesPerThread * i]);
        m_TreeStates[i].nodes = reinterpret_cast<TreeNode *>(m_Arena[nodeBytesOffset + nodeBytesPerThread * i]);

        m_Threads.EmplaceBack([this, i] { Worker::EntryPoint(*this, i); });
    }

    // Initialise our board
    fen::Parse(fen, nullptr, nullptr, m_State);
    m_State.zobristKey = m_Zobrist.ComputeKey(m_State);
    m_State.pawnKey = m_Zobrist.ComputePawnKey(m_State);
    for (Color::Value color : { Color::WHITE, Color::BLACK }) {
        for (Piece::Value piece = Piece::PAWN; piece < Piece::MAX_ENUM; piece++) {
            Bitboard occupancy = m_State.pieces.Occupancy(color, piece);
            while (occupancy) {
                uint8_t index = std::countr_zero(occupancy);
                m_State.pstScore += m_PieceSquareTables.Get(color, piece, index);
                occupancy &= (occupancy - 1);
            }
        }
    }

    SendMessage(Command::STATE, MANAGER_THREAD_ID);
}

DTSManager::~DTSManager()
{
    delete[] m_Arena;
}

void DTSManager::SetTimeControl(float seconds, float increment)
{
    m_Searcher.SetTimeControl(seconds, increment);
}

void DTSManager::SetWeights(const EvaluatorConstants& weights)
{
    m_Searcher.TuneEval(weights);
}

Move DTSManager::FindBest(uint64_t msLeft)
{
    // TODO: Multithreaded!
    return Move::Invalid();
}

void DTSManager::MakeMove(Move move)
{
    m_State.MakeMove(m_Zobrist, m_PieceSquareTables, move);
    SendMessage(Command::STATE, MANAGER_THREAD_ID);
}

bool DTSManager::HasMessage(ThreadId me)
{
    return !m_Queues[me]->Empty();
}

Message DTSManager::ReadMessage(ThreadId me)
{
    return m_Queues[me]->Dequeue();
}

void DTSManager::SendMessage(Command::Value command, ThreadId me)
{
    for (std::size_t i = 0; i < DTS_MAX_THREADS; i++) {
        if (i == me) continue;
        
        SendMessageTo(command, me, i);
    }
}

void DTSManager::SendMessageTo(Command::Value command, ThreadId me, ThreadId to)
{
    Message message;
    message.command = command;
    message.sentBy = me;

    switch (command) {
        case Command::STATE:
            if (me != MANAGER_THREAD_ID) ERROR("Only the manager should send STATE commands");
            message.state = &m_State;
            break;
        case Command::HELP: // Ask the receiver to copy their tree to the provided memory
            message.helpBlock = &m_TreeStates[to];
            break;
        default:
            ERROR("Command not implemented (" << command << ")");
            std::unreachable();
    }

    m_Queues[to]->Enqueue(message);
}

