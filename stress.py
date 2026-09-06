#!/usr/bin/env python3

from time import sleep

from lib.build.libjupiter import Board

MS_REMAINING: int = 500 * 1000

FEN: str = "1nk1r1r1/pp2n1pp/4p3/q2pPp1N/b1pP1P2/B1P2R2/2P1B1PP/R2Q2K1 w - - 0 1"

def main():
    board: Board = Board(FEN)
    board.set_time_control(MS_REMAINING // 1000, 0)
    sleep(1)
    move: str | None = board.go(MS_REMAINING)
    if move is None:
        raise ValueError("Failed to generate move")

    board.make_move(move)

if __name__ == "__main__":
    main()
