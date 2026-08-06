#pragma once 

#include "boardState.h"
#include "pieceSquareTable.h"
#include <cstdint>

const int64_t MATE_EVAL = 100'000'000;
const int64_t MATE_THRESOLD = 99'000'000;

struct SEE {
    typedef enum {
        LOSING,
        WINNING,
        EQUAL
    } Value;
};

class Evaluator {
public:
    Evaluator() = default;
    int64_t Evaluate(const BoardState& state);
    uint8_t GamePhase(const BoardState& state);

private:
    int64_t MaterialBalance(const BoardState& state);
    int64_t PiecePositions(const BoardState& state);

private:
    PieceSquareTableManager m_PieceSquareTables{PieceSquareTableManager()};
};
