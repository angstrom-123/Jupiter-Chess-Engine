#pragma once

#include "evaluation/evaluator.h"
#include "search/transpositionTable.h"
#include <cstdint>

struct AlphaBeta {
    int32_t alpha{-INFINITY_EVAL};
    int32_t beta{INFINITY_EVAL};

    AlphaBeta() = default;
    AlphaBeta(int32_t alpha, int32_t beta)
        : alpha{alpha}, beta{beta} {}

    INLINE AlphaBeta operator-() const { return AlphaBeta(-beta, -alpha); }
    INLINE AlphaBeta Null() const { return AlphaBeta(-alpha - 1, -alpha); }
    INLINE bool Contains(int32_t score) const { return score > alpha && score < beta; }
    INLINE bool IsBetaCutoff() const { return alpha >= beta; }
    INLINE void Update(Cutoff::Value cutoff, int32_t score)
    {
        // NOTE: This seems like it's flipped, but this is actually correct.
        //       If a beta cutoff occurred, then the score is a lower bound on 
        //       the true score for that node. Since it is a lower bound, it 
        //       should raise alpha. And vice versa for lowering beta. 
        if (cutoff == Cutoff::BETA)
            alpha = std::max(alpha, score);
        else if (cutoff == Cutoff::ALPHA)
            beta = std::min(beta, score);
    }
};

