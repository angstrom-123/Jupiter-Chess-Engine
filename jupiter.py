#!/usr/bin/env python3

import json
from typing import override

# Absolute import here is ok since this is in the same dir as the server cwd
from framework.base_engine import BaseEngine, TimeControl

# Use relative imports here because this file will be loaded from a different directory
from .jupiterengine.build.libjupiter import Board
from . import helpers

class Jupiter(BaseEngine):
    board: Board | None = None 
    n_searches: int = 0
    telemetry: dict[str, int] = {}

    @override
    def init(self, tc: TimeControl, fen: str | None = None) -> None:
        self.board = Board() if fen is None else Board(fen)
        self.board.set_time_control(tc.seconds, tc.increment)
        self.n_searches = 0 
        self.telemetry = {}

    @helpers.not_none("board")
    @override
    def go(self, ms_left: int) -> str | None:
        move: str | None = self.board.go(ms_left)
        self.n_searches += 1
        telem: dict[str, int] = json.loads(self.board.get_telemetry())
        self.telemetry = telem if len(self.telemetry.keys()) == 0 else { k: self.telemetry[k] + v for k, v in telem.items()}
        return move

    @helpers.not_none("board")
    @override
    def move(self, move: str) -> None:
        self.board.make_move(move)

    @helpers.not_none("board")
    @override 
    def tuning_get_params(self) -> dict[str, float]:
        weights: dict[str, float | int] = self.board.get_weights()
        return helpers.normalise_weights(weights)

    @helpers.not_none("board")
    @override 
    def tuning_set_params(self, params: dict[str, float]) -> None:
        weights: dict[str, float] = helpers.denormalise_weights(params)
        print("Set:")
        print(weights)
        helpers.assign_weights(self.board, weights)

    @helpers.not_none("board")
    @override 
    def game_over(self) -> None:
        metrics: dict[str, int] = json.loads(self.board.get_metrics())
        helpers.print_telemetry_and_metrics(self.telemetry, metrics, self.n_searches)

    @helpers.not_none("board")
    @override 
    def show(self) -> str:
        return repr(self.board)

    @helpers.not_none("board")
    @override 
    def __repr__(self) -> str:
        return repr(self.board)
