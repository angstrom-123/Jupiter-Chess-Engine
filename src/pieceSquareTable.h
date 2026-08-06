#pragma once

#include "boardState.h"
#include "core.h"
#include <cstdint>

struct PieceSquareTable {
    int64_t midgameTable[64];
    int64_t endgameTable[64];
};

class PieceSquareTableManager {
public:
    PieceSquareTableManager();
    int64_t Get(Color::Value color, Piece::Value piece, uint8_t index, uint8_t phase);

private:
    PieceSquareTable m_Tables[Piece::MAX_ENUM];
};
