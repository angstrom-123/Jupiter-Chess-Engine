#pragma once

#include <cstdint>
#include <string>

#include "board/boardState.h"
#include "board/history.h"
#include "evaluation/pieceSquareTable.h"
#include "movegen/move.h"
#include "search/searcher.h"

namespace libjupiter {
    class Board {
    public:
        Board(const char *fen);
        void SetTimeControl(uint64_t seconds, uint64_t increment);
        Move Go(uint64_t moveMs);
        void MakeMove(LongAlgebraicMove lan);
        void Show(std::string& result);
        void GetTelemetry(std::string& result);
        void GetMetrics(std::string& result);

    private:
        void ComputePSTScore();
        void Clear();

    private:
        History m_History{History()};
        Zobrist m_Zobrist{Zobrist()};
        PieceSquareTables m_PieceSquareTables{PieceSquareTables()};
        Searcher m_Searcher{Searcher(m_Zobrist, m_PieceSquareTables)}; // TODO: Add opening book and tablebase to search
        BoardState m_State;
        uint64_t m_FullMoves{1};
        uint64_t m_HalfMoves{0};
    };
}
