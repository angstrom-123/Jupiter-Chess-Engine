#include "history.h"

bool History::IsRepetition()
{
    JUPITER_TRACE();

    ZobristKey current = m_History[m_History.Size() - 1];
    for (int64_t i = m_History.Size() - 2; i >= 0; i--) {
        if (m_History[i] == current)
            return true;
    }
    return false;
}

void History::Push(const BoardState& state)
{
    JUPITER_TRACE();

    m_History.PushBack(m_Zobrist.ComputeKey(std::forward<const BoardState>(state)));
}

void History::Pop()
{
    JUPITER_TRACE();

    m_History.PopBack();
}
