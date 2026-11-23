#pragma once

#include "board/history.h"
#include "types.h"
#include "treeState.h"

// Worker passes itself to the search call it makes?
// Becuase IT must call search!
class Worker {
public:
    Worker(class DTSManager& manager, ThreadId id)
        : m_Manager{manager}, m_Index{id} {};

    // For constructing the tree during search
    TreeNode& PushNode();
    void PopNode();

    // Entry point for this worker, manager will call this to spawn
    static void EntryPoint(class DTSManager& manager, ThreadId id);

private:
    void CopyOutTreeState(TreeState& result);

private:
    DTSManager& m_Manager;
    History m_History;

    ThreadId m_Index{THREAD_ID_MAX};
    std::size_t m_CurrentPly{0};
    TreeNode m_Nodes[MAX_PLY];
};
