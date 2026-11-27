#pragma once

#include <cstdint>
#include <string>

#include "dts/manager.h"
#include "evaluation/evaluator.h"
#include "movegen/move.h"
#ifndef __clang__
    #include "util/nullables.h"
#endif

namespace libjupiter {
    class Board {
    public:
        Board(const char *_Nullable fen);
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
        DTSManager *_Nonnull m_DTSManager{nullptr};
    };
}
