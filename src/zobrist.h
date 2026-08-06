#pragma once

#include "boardState.h"

constexpr uint64_t ZOBRIST_NUMBER_COUNT = 781;

using ZobristKey = uint64_t;

// TODO: Incremental key changes with moves for efficiency in make/unmake
class Zobrist {
public: 
    Zobrist();
    ZobristKey ComputeKey(const BoardState& state) const;

private:
    uint64_t m_Randoms[ZOBRIST_NUMBER_COUNT];
};
