#!/usr/bin/env python3

import json
from typing import override

from framework.base_engine import BaseEngine, TimeControl

from .lib.build.libjupiter import Board

class Jupiter(BaseEngine):
    board: Board | None = None 
    n_searches: int = 0
    telemetry: dict = {}

    @override
    def init(self, tc: TimeControl, fen: str | None = None) -> None:
        self.board = Board() if fen is None else Board(fen)
        self.board.set_time_control(tc.seconds, tc.increment)

    @override
    def go(self, ms_left: int) -> str | None:
        if self.board is None:
            raise AttributeError("[JUPITER] self.board is not initialised. Try calling init.")

        move: str | None = self.board.go(ms_left)
        self._save_metrics()
        return move

    @override
    def move(self, move: str) -> None:
        if self.board is None:
            raise AttributeError("[JUPITER] self.board is not initialised. Try calling init.")

        self.board.make_move(move)

    @override 
    def game_over(self) -> None:
        if self.board is None:
            raise AttributeError("[JUPITER] self.board is not initialised. Try calling init.")

        metrics: dict = json.loads(self.board.get_metrics())

        if self.telemetry["searchTime"] == 0:
            print("[JUPITER] No metrics to show - never left opening book")
            return

        print(f"""
[JUPITER] Game metrics:
    - avg Nodes Searched  : {(self.telemetry["nodesSearched"] / self.n_searches) / 1_000_000:.3f}M
    - avg Nodes Quiesced  : {(self.telemetry["nodesQuiesced"] / self.n_searches) / 1_000_000:.3f}M
    - avg Search Speed    : {self.telemetry["nodesSearched"] / (self.telemetry["searchTime"] * 1000):.3f}mnps 
    - avg Quiescence %    : {(self.telemetry["nodesQuiesced"] / self.telemetry["nodesSearched"]) * 100:.3f}%
    - avg Lookup %        : {(self.telemetry["nodesLookedUp"] / self.telemetry["nodesSearched"]) * 100:.3f}%
    - avg Completed Depth : {self.telemetry["depth"] / self.n_searches:.3f}
    - TT Occupancy        : {metrics["ttSize"] / (1024 * 1024):.3f}MiB
    - Book Moves          : {metrics["bookMoves"]}
    - Searches Completed  : {self.n_searches}
""")

    @override 
    def show(self) -> str:
        return repr(self.board)

    @override 
    def __repr__(self) -> str:
        return repr(self.board)

    def _save_metrics(self):
        if self.board is None:
            raise AttributeError("[JUPITER] self.board is not initialised. Try calling init.")

        self.n_searches += 1
        telem: dict = json.loads(self.board.get_telemetry())
        if len(self.telemetry.keys()) == 0:
            self.telemetry = telem
        else:
            self.telemetry = { k: self.telemetry[k] + telem[k] for k in self.telemetry.keys()}
