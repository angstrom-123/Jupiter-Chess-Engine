#pragma once

#include <cstdint>
#include <string>

#include "core.h"
#include "board/boardState.h"
#include "board/history.h"
#include "evaluation/pieceSquareTable.h"
#include "movegen/move.h"
#include "search/openingBook.h"
#include "search/searcher.h"

namespace libjupiter {
    class Board {
    public:
        Board(const char *fen);
        ~Board();
        void SetTimeControl(uint64_t seconds, uint64_t increment);
        Move Go(uint64_t moveMs);
        void MakeMove(LongAlgebraicMove lan);
        void Show(std::string& result);
        void GetTelemetry(std::string& result);
        void GetMetrics(std::string& result);

    private:
        void ComputePSTScore();
        void Clear();
        bool SplitFEN(const char *fen, uint64_t length, FenView (&views)[13]);

    private:
        History m_History{History()};
        Zobrist m_Zobrist{Zobrist()};
        OpeningBook m_OpeningBook{OpeningBook()};
        PieceSquareTables m_PieceSquareTables{PieceSquareTables()};
        Searcher m_Searcher{Searcher(m_Zobrist, m_OpeningBook, m_PieceSquareTables)};
        BoardState m_State;
        uint64_t m_FullMoves{1};
        uint64_t m_HalfMoves{0};
    };
}
