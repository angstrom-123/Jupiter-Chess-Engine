#!/usr/bin/env python3

from lib.build.libjupiter import Board

MS_REMAINING: int = 100 * 1000

FEN: list[str] = [
    "1nk1r1r1/pp2n1pp/4p3/q2pPp1N/b1pP1P2/B1P2R2/2P1B1PP/R2Q2K1 w - - 0 1",
    # "4b3/p3kp2/6p1/3pP2p/2pP1P2/4K1P1/P3N2P/8 w - - 0 1",
    # "2kr1bnr/pbpq4/2n1pp2/3p3p/3P1P1B/2N2N1Q/PPP3PP/2KR1B1R w - - 0 1"
]

def main():
    for position in FEN:
        board: Board = Board(position)
        board.set_time_control(MS_REMAINING // 1000, 0)
        move: str | None = board.go(MS_REMAINING)
        if move is None:
            raise ValueError("Failed to generate move")

        board.make_move(move)

if __name__ == "__main__":
    main()
