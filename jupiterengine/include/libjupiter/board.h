#pragma once

#include <cstdint>
#include <string>

#include "board/boardState.h"
#include "board/history.h"
#include "evaluation/evaluator.h"
#include "evaluation/pieceSquareTable.h"
#include "movegen/move.h"
#include "search/searcher.h"

namespace libjupiter {
    class Board {
    public:
        Board(const char *fen);
        ~Board();
        void SetTimeControl(float seconds, float increment);
        Move Go(uint64_t moveMs);
        void MakeMove(LongAlgebraicMove lan);
        void Show(std::string& result) const;
        void GetTelemetry(std::string& result) const;
        void GetMetrics(std::string& result) const;
        void SetWeights(const EvaluatorConstants& weights);
        EvaluatorConstants GetWeights() const;

    private:
        void ComputePSTScore();
        void StartPos();

    private:
        History m_History{History()};
        Zobrist m_Zobrist{Zobrist()};
        PieceSquareTables m_PieceSquareTables{PieceSquareTables()};
        Searcher *m_Searcher{new Searcher(m_Zobrist, m_PieceSquareTables)}; // TODO: Add opening book and tablebase to search
        BoardState m_State;
        uint64_t m_FullMoves{1};
        uint64_t m_HalfMoves{0};
    };
}
