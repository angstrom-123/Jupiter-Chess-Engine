#pragma once

#include <cstdint>
#include <string>

#include "boardState.h"
#include "core.h"
#include "move.h"
#include "openingBook.h"
#include "searcher.h"
#include "history.h"

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
        void Clear();
        bool SplitFEN(const char *fen, uint64_t length, FenView (&views)[13]);

    private:
        Zobrist m_Zobrist;
        OpeningBook m_OpeningBook;
        History m_History;
        Searcher m_Searcher;
        BoardState m_State;
        uint64_t m_FullMoves{1};
    };
}
