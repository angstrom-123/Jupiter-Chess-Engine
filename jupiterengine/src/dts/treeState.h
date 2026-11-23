#pragma once

#include "evaluation/evaluator.h"
#include "movegen/moveStream.h"
#include "search/window.h"

// TODO:
//      Need to construct trees as we search.
//      Every recursive call needs to allocate a tree node and fill in its data

// TODO: Store the tree nodes
//          Arena allocator?
//          Bump allocator?
//          Per Thread memory block?
struct TreeNode {
    TreeNode *child{nullptr};
    int32_t score{-INFINITY_EVAL};
    AlphaBeta window{};
    MoveStream moves;
};
static_assert(std::is_trivially_destructible<TreeNode>() && "If this doesn't hold then we should clean up when deallocating it in the worker");

// Prevent false sharing!
struct alignas(64) TreeState {
    TreeNode *nodes{nullptr};
    std::size_t count{0};
};
