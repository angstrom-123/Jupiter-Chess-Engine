#include "worker.h"
#include "manager.h"
#include "util/instrumenter.h"
#include "util/pause.h"

void Worker::EntryPoint(class DTSManager& manager, ThreadId id)
{
    JUPITER_TRACE();

    Worker w(manager, id);

    while (1) {
        Message message = manager.WorkerReadMessage(id);

        w.HandleMessage(message);

        if (manager.WorkerShouldStop(id))
            break;
    }
}

Worker::Worker(class DTSManager& manager, ThreadId id)
    : m_Manager{manager}, m_Index{id} 
{
    JUPITER_TRACE();

    // Initialise our board, the manager gives us a minimal board state that 
    // doesn't include zobrist hashes or pst scores so we compute them here.
    m_State = manager.state;
    m_State.pstScore = m_PieceSquareTables.ComputeScore(m_State.pieces);
    m_State.zobristKey = m_Zobrist.ComputeKey(m_State);
    m_State.pawnKey = m_Zobrist.ComputePawnKey(m_State);
}

void Worker::HandleMessage(const Message& message)
{
    JUPITER_TRACE();

    switch (message.command) {
        case Command::NONE:
            CpuPause();
            break;
        case Command::TUNE:
            m_Eval.SetWeights(*message.weights);
            break;
        case Command::GO:
            Go();
            break;
        case Command::WORKER_HELP:
            CopyOutTreeState(message.helpBlock);
            break;
    }
}

void Worker::CopyOutTreeState(TreeState *result)
{
    JUPITER_TRACE();

    std::memcpy(m_Nodes, result->nodes, sizeof(TreeNode) * m_CurrentPly);
    result->count = m_CurrentPly;
}

void Worker::Go()
{
    JUPITER_TRACE();

    // TODO: Iterative deepening

    uint16_t depth = 5;
    RecurseFirstChild(depth, 0);

    // TODO: This is just temporary
    m_Manager.WorkerUpdateBestMove(Move(56, 48, Piece::PAWN));
    m_Manager.WorkerSetSearchComplete();
}

void Worker::RecurseFirstChild(uint16_t depth, uint16_t ply)
{
    JUPITER_TRACE();

    // TODO: Should I throttle this check?
    if (m_Manager.WorkerShouldStop(m_Index)) {
        INFO("Thread was requested to stop before reaching the bottom of recursion!");
        return;
    }

    if (depth == 0) {
        INFO("Reached the bottom of the recursion!");
        // TODO: Bring in the other threads!
        return;
    }

    TreeNode& node = m_Nodes[ply];
    node.moves.Init(m_State, m_AttackTable, m_Killers[std::min(ply, MAX_PLY)], false);

    Move leftmost;
    while ((leftmost = node.moves.Stream(m_State, m_Eval, m_HistoryTable)).IsValid()) {
        MoveData moveData = m_State.MakeMove(m_Zobrist, m_PieceSquareTables, leftmost);
        if (m_State.WasLegalMove(m_AttackTable, moveData)) {
            RecurseFirstChild(depth - 1, ply + 1);

            // Break the loop after the first legal move
            m_State.UnmakeMove(moveData);
            break;
        }

        // Streamed move was not legal so we need to try the next one
        m_State.UnmakeMove(moveData);
    }
}
