#pragma once

#include "board/history.h"
#include "dts/messageQueue.h"
#include "types.h"
#include "treeState.h"

// Worker passes itself to the search call it makes?
// Becuase IT must call search!
class Worker {
public:
    Worker(class DTSManager& manager, ThreadId id);

    // Entry point for this worker, manager will call this to spawn
    static void EntryPoint(class DTSManager& manager, ThreadId id);

private:
    void HandleMessage(const Message& message);
    void CopyOutTreeState(TreeState *result);
    void Go();
    void RecurseFirstChild(uint16_t depth, uint16_t ply);

private:
    DTSManager& m_Manager;
    ThreadId m_Index;

    History m_History{};
    HistoryTable m_HistoryTable{}; // TODO: actually implement this
    KillerMoveBuffer m_Killers{}; // TODO: actually implement this
    AttackTable m_AttackTable{};
    Evaluator m_Eval{m_AttackTable};
    BoardState m_State{};
    PieceSquareTables m_PieceSquareTables{};
    Zobrist m_Zobrist{};
    std::size_t m_CurrentPly{0};
    TreeNode m_Nodes[MAX_PLY];
};
