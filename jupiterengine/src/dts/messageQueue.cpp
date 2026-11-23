#include "messageQueue.h"

bool MessageQueue::Empty()
{
    m_Lock.lock();

    std::size_t size = m_Queue.Size(); // Could be atomic, probably fine

    m_Lock.unlock();

    return size > 0;
}

void MessageQueue::Enqueue(const Message& message)
{
    m_Lock.lock();

    m_Queue.Push(message);

    m_Lock.unlock();
}

Message MessageQueue::Dequeue()
{
    Message m;

    m_Lock.lock();

    m = m_Queue.Peek(); // Copy
    m_Queue.Pop();

    m_Lock.unlock();

    return m;
}
