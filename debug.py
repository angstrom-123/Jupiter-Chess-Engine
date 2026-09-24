#!/usr/bin/env python3

import json

from jupiterengine.build.libjupiter import Board
from helpers import denormalise_weights, assign_weights

board: Board = Board("8/8/1kp5/2p5/2ppp2P/7P/P3PK2/8 w - - 0 1")

weights = {
    "material_weight": 0.8991150552298602,
    "pst_weight": 0.41603960381524296,
    "mopup_proximity_factor": -0.16725139238580378,
    "mopup_edge_factor": 0.28504942391066634,
    "king_mobility_factor":	-0.22357966979910618,
    "mobility_factor": 0.12925671194510954,
    "king_pawn_tropism_regular_factor":	-0.11836980155952265,
    "king_pawn_tropism_weak_factor": 0.300877706269155,
    "king_pawn_tropism_passed_factor": 0.21847747195030476,
    "missing_shield_pawn_factor": -0.28571919703989335,
    "storming_pawn_factor":	-0.11967683013618044,
    "slider_open_file_factor": 0.4384669289695806,
    "weak_pawn_factor":	-0.22724635994739126,
    "connected_pawn_factor": 0.9938567030945792
}

assign_weights(board, denormalise_weights(weights))
print(board.get_weights())

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
