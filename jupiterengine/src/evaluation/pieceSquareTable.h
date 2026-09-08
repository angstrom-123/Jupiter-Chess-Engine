#pragma once

#include "core.h"
#include <cstdint>

struct PSTScore {
    int16_t midgame{0};
    int16_t endgame{0};

    PSTScore(int16_t midgame, int16_t endgame)
        : midgame{midgame}, endgame{endgame} {}

    PSTScore& operator-=(const PSTScore& other) 
    {
        midgame -= other.midgame;
        endgame -= other.endgame;
        return *this;
    }

    PSTScore& operator+=(const PSTScore& other) 
    {
        midgame += other.midgame;
        endgame += other.endgame;
        return *this;
    }

    PSTScore operator-() const 
    {
        return PSTScore(-midgame, -endgame);
    }
};

struct PieceSquareTable {
    int64_t midgameTable[64];
    int64_t endgameTable[64];
};

class PieceSquareTables {
public:
    PieceSquareTables();
    [[nodiscard]] PSTScore Get(Color::Value color, Piece::Value piece, uint8_t index) const;

private:
    PieceSquareTable m_Tables[Piece::MAX_ENUM];
};
