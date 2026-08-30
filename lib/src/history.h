#pragma once

#include "zobrist.h"
#include <vector>

class History {
public:
    History();
    bool IsRepetition();
    void Push(const BoardState& state);
    void Pop();

private:
    std::vector<ZobristKey> m_History;
};
