#include "board/history.h"
#include "util/instrumenter.h"
#include "board/boardState.h"

History::History()
{
    m_History.reserve(512);
}

bool History::IsRepetition()
{
    JUPITER_TRACE();
    JUPITER_PROFILE();

    if (m_History.size() <= 2)
        return false;

    ZobristKey current = m_History[m_History.size() - 1];
    for (int64_t i = m_History.size() - 2; i >= 0; i--) {
        if (m_History[i] == current)
            return true;
    }
    return false;
}

void History::Push(const BoardState& state)
{
    JUPITER_TRACE();

    m_History.push_back(state.zobristKey);
}

void History::Pop()
{
    JUPITER_TRACE();

    m_History.pop_back();
}
