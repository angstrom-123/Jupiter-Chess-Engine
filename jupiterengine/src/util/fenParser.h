#pragma once 

#include "board/boardState.h"

namespace fen {
    void Parse(const char *_Nonnull fen, uint64_t *_Nullable halfMoveRes, uint64_t *_Nullable fullMoveRes, BoardState& res);
};
