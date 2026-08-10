#pragma once

#include <cstddef>
#include "buffer.h"
#include "move.h"
#include "zobrist.h"

using OpeningMoves = Buffer<std::pair<Move, uint16_t>, 5>;

class OpeningBook {
public:
    OpeningBook();
    bool LookupMoves(const BoardState& state, OpeningMoves& moves);

private:
    std::pair<Move, uint16_t> ParseMove(const BoardState& state, uint64_t bits);
    ZobristKey ZobristHash(const BoardState& state);

private: 
    std::vector<uint8_t> m_Bytes;
    std::size_t m_FileSizeBytes{0};
};
