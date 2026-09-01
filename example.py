#!/usr/bin/env python3

import json

from lib.build.libjupiter import Board

board: Board = Board("kb/8/8/4KR2/8/8/8/8 w - - 0 1")

print(repr(board))

telemetry: dict = {}
n_searches: int = 0

increment: int = 1
seconds: int = 10
board.set_time_control(seconds, increment)

time_remaining_ms: int = 5000

while True:
    best_move: str | None = board.go(time_remaining_ms)
    if best_move is None:
        print("done")
        break 

    board.make_move(best_move)
    n_searches += 1
    telem: dict = json.loads(board.get_telemetry())
    if len(telemetry.keys()) == 0:
        telemetry = telem
    else:
        telemetry = { k: telemetry[k] + telem[k] for k in telemetry.keys()}
    print(repr(board))

if telemetry["searchTime"] == 0:
    print("[JUPITER] No metrics to show - never left opening book")
else:
    metrics: dict = json.loads(board.get_metrics())

    print(f"""
[JUPITER] Game metrics:
    - avg Nodes Searched  : {(telemetry["nodesSearched"] / n_searches) / 1_000_000:.3f}M
    - avg Nodes Quiesced  : {(telemetry["nodesQuiesced"] / n_searches) / 1_000_000:.3f}M
    - avg Search Speed    : {telemetry["nodesSearched"] / (telemetry["searchTime"] * 1000):.3f}mnps 
    - avg Quiescence %    : {(telemetry["nodesQuiesced"] / telemetry["nodesSearched"]) * 100:.3f}%
    - avg Lookup %        : {(telemetry["nodesLookedUp"] / telemetry["nodesSearched"]) * 100:.3f}%
    - avg Completed Depth : {telemetry["depth"] / n_searches:.3f}
    - TT Occupancy        : {metrics["ttSize"] / (1024 * 1024):.3f}MiB
    - Book Moves          : {metrics["bookMoves"]}
    - Searches Completed  : {n_searches}
""")

print(repr(board))
