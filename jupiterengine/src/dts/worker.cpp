#include "worker.h"

void Worker::EntryPoint(class DTSManager& manager, ThreadId id)
{
    Worker w(manager, id);

    while (1) {
        // TODO: Read the queue from the manager
    }
}

void Worker::CopyOutTreeState(TreeState& result)
{
    std::memcpy(m_Nodes, result.nodes, sizeof(TreeNode) * m_CurrentPly);
    result.count = m_CurrentPly;
}

TreeNode& Worker::PushNode()
{
    return m_Nodes[m_CurrentPly++];
}

void Worker::PopNode()
{
    // TreeNode should be trivially destructible so won't cause a problem if we 
    // don't call its destructor.
    --m_CurrentPly;
}
