#pragma once

#include <cstdint>
#include <string>

#include "dts/manager.h"
#include "evaluation/evaluator.h"
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
		uint64_t Perft(uint8_t depth);
		uint64_t PerftDivide(uint8_t depth, PerftBuffer& buffer);
        EvaluatorConstants GetWeights() const;

    private:
        DTSManager *m_DTSManager{nullptr};
    };
}
