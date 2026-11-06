#!/usr/bin/env python3

import json
import time

from jupiterengine.build.libjupiter import Board
from helpers import denormalise_weights, assign_weights

# board = Board()
# depth: int = 7
# n, div = board.perft_divide(depth)
# print(f"Perft divide ({depth}): {n} moves:")
# for k, v in sorted(div.items()):
#     print(f"{k}: {v}")

perft = [
    # Startpos
    # ("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", 1, 20),
    # ("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", 2, 400),
    # ("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", 3, 8902),
    # ("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", 4, 197281),
    # ("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", 5, 4865609),
    # ("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", 6, 119060324),
    ("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", 7, 3195901860),
    # ("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", 8, 84998978956),
    # ("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", 9, 2439530234167),

    # Kiwipete
    # ("r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1", 1, 48),
    # ("r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1", 2, 2039),
    # ("r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1", 3, 97862),
    # ("r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1", 4, 4085603),
    # ("r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1", 5, 193690690),
    # ("r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1", 6, 8031647685),
    # ("r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1", 7, 374190009323),
]

for (pos, depth, expected) in perft:
    board: Board = Board(pos)

    start = time.perf_counter()

    n, div = board.perft_divide(depth)

    end = time.perf_counter()

    print(f"Finished perft run in {end - start:.2f}s ({(n / 1000000) / (end - start)}mnps)")
    print(f"Perft({depth}) on '{pos}' gave {n} moves")
    print(f"    Expected: {expected}")
    print(f"    Ok?: {'true' if n == expected else 'false'}")

# telemetry: dict = {}
# n_searches: int = 0
#
# board.set_time_control(10, 1)
#
# while True:
#     best_move: str | None = board.go(1000)
#     if best_move is None:
#         print("done")
#         break 
#
#     board.make_move(best_move)
#     n_searches += 1
#     telem: dict = json.loads(board.get_telemetry())
#     telemetry = telem if len(telemetry.keys()) == 0 else { k: telemetry[k] + telem[k] for k in telemetry.keys()}
#     print(repr(board))
#
# if telemetry["searchTime"] == 0:
#     print("[JUPITER] No metrics to show - never left opening book")
# else:
#     metrics: dict = json.loads(board.get_metrics())
#
#     print(f"""
# [JUPITER] Game metrics:
#     - avg Nodes Searched  : {(telemetry["nodesSearched"] / n_searches) / 1_000_000:.3f}M
#     - avg Nodes Quiesced  : {(telemetry["nodesQuiesced"] / n_searches) / 1_000_000:.3f}M
#     - avg Search Speed    : {telemetry["nodesSearched"] / (telemetry["searchTime"] * 1000):.3f}mnps 
#     - avg Quiescence %    : {(telemetry["nodesQuiesced"] / telemetry["nodesSearched"]) * 100:.3f}%
#     - avg Lookup %        : {(telemetry["nodesLookedUp"] / telemetry["nodesSearched"]) * 100:.3f}%
#     - avg Completed Depth : {telemetry["depth"] / n_searches:.3f}
#     - TT Occupancy        : {metrics["ttSize"] / (1024 * 1024):.3f}MiB
#     - Book Moves          : {metrics["bookMoves"]}
#     - Searches Completed  : {n_searches}
# """)
#
# print(repr(board))
