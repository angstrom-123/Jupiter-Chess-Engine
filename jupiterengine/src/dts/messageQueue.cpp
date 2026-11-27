#include "messageQueue.h"

void MessageQueue::Enqueue(const Message& message)
{
    JUPITER_TRACE();

    m_Lock.lock();

    m_Queue.Push(message);

    m_Lock.unlock();
}

Message MessageQueue::TryDequeue()
{
    JUPITER_TRACE();

    Message m;

    m_Lock.lock();

    if (m_Queue.Size() > 0) {
        m = m_Queue.Peek(); // Copy
        m_Queue.Pop();
    } else {
        m.command = Command::NONE;
    }

    m_Lock.unlock();

    return m;
}
