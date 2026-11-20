#!/usr/bin/env python3

import json
import time
import sys

from jupiterengine.build.libjupiter import Board

def main():
    if len(sys.argv) != 2:
        help()
        return

    match sys.argv[1]:
        case "help":
            help()
        case "test":
            test()
        case "perft":
            perft()
        case "stress":
            stress()
        case "bench":
            benchmark()

def help():
    print("""
JUPITER DEBUG SCRIPT

Commands:
    help            -           print this help information.
    test            -           run the test script.
    bench           -           run the performance benchmark script.
    perft           -           run the movegen test script.
    stress          -           run the stress test script.
""")

def benchmark():
    board = Board("r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1")

    n_tests = 10
    total_nodes = 0.0
    total_time = 0.0

    for i in range(n_tests):
        move = board.go(60_000)
        if move is None:
            raise RuntimeError("go call returned None")
        board.make_move(move)

        telem = json.loads(board.get_telemetry())
        total_nodes += telem["nodesSearched"]
        total_time += telem["searchTime"]

        print(f"({i}/{n_tests})", end="\r")

    print(f"Average search speed ({n_tests} iterations): {total_nodes / (total_time * 1000.0):.3f}mnps")

def stress():
    board = Board("r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1")
    time.sleep(1)
    board.go(60_000)

def test():
    fen = "1K6/5q2/8/7p/7P/5k2/2n5/8 w - - 0 1"

    board = Board(fen)
    print(repr(board))

    move = board.go(1000)
    print(move)

    if move is None:
        return

    board.make_move(move)
    print(repr(board))


def perft():
    perft = [
        # Startpos
        ("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", 1, 20),
        ("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", 2, 400),
        ("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", 3, 8902),
        ("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", 4, 197281),
        ("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", 5, 4865609),
        ("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", 6, 119060324),
        # ("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", 7, 3195901860),
        # ("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", 8, 84998978956),
        # ("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", 9, 2439530234167),

        # Kiwipete
        ("r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1", 1, 48),
        ("r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1", 2, 2039),
        ("r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1", 3, 97862),
        ("r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1", 4, 4085603),
        ("r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1", 5, 193690690),
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

if __name__ == "__main__":
    main()

