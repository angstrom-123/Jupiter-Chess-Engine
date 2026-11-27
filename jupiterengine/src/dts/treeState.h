#pragma once

#include "evaluation/evaluator.h"
#include "movegen/moveStream.h"
#include "search/window.h"

struct TreeNode {
    MoveStream moves{};
    AlphaBeta window{};
    int32_t score{-INFINITY_EVAL};
};

// Prevent false sharing!
struct alignas(64) TreeState {
    TreeNode *nodes{nullptr};
    std::size_t count{0};
};
