#!/usr/bin/env python3

import json
import math
import sys

from lib.build.libjupiter import Board

def process_results(filename: str | None, cmp_filename: str | None, telemetry_list: list[str], metrics_list: list[str]):
    # Aggregate data
    telemetry: dict = {}
    tmp: dict = {}
    for t in telemetry_list:
        tmp = json.loads(t)
        telemetry = tmp if len(telemetry.keys()) == 0 else { key: int(tmp[key]) + int(telemetry[key]) for key in telemetry.keys() }

    metrics: dict = {}
    for m in metrics_list:
        tmp = json.loads(m)
        tmp.pop("bookMoves")
        metrics = tmp if len(metrics.keys()) == 0 else { key: int(tmp[key]) + int(metrics[key]) for key in metrics.keys() }

    # Compute averages
    n_telem: int = len(telemetry_list)
    n_metri: int = len(metrics_list)
    search_speed_avg: float = telemetry["nodesSearched"] / (telemetry["searchTime"] * 1000) # mnps
    quiescence_percent_avg: float = (telemetry["nodesQuiesced"] / telemetry["nodesSearched"]) * 100
    lookup_percent_avg: float = (telemetry["nodesLookedUp"] / telemetry["nodesSearched"]) * 100
    depth_completed_avg: float = telemetry["depth"] / n_telem
    tt_occupancy_avg: float = metrics["ttSize"] / (1024 * 1024 * n_metri)

    # Print averages and compare to prior results if file provided
    print("Averages:")
    if cmp_filename is not None:
        with open(cmp_filename, "r") as f:
            cmp: dict = json.loads(f.read())
        
        search_speed_delta: float = -(100 - (search_speed_avg / float(cmp["search_speed"])) * 100) if float(cmp["search_speed"]) > 0 else search_speed_avg 
        quiescence_percent_delta: float = -(100 - (quiescence_percent_avg / float(cmp["quiescence_percent"])) * 100) if float(cmp["quiescence_percent"]) > 0 else quiescence_percent_avg 
        lookup_percent_delta: float = -(100 - (lookup_percent_avg / float(cmp["lookup_percent"])) * 100) if float(cmp["lookup_percent"]) > 0 else lookup_percent_avg 
        depth_completed_delta: float = -(100 - (depth_completed_avg / float(cmp["depth_completed"])) * 100) if float(cmp["depth_completed"]) > 0 else depth_completed_avg 
        tt_occupancy_delta: float = -(100 - (tt_occupancy_avg / float(cmp["tt_occupancy"])) * 100) if float(cmp["tt_occupancy"]) > 0 else tt_occupancy_avg

        print(f"  Search Speed ..... {f"{search_speed_avg:.2f} Mnps":12}({"+" if search_speed_delta > 0 else ""}{search_speed_delta:.2f}%)")
        print(f"  % Quiesced ....... {f"{quiescence_percent_avg:.2f}%":12}({"+" if quiescence_percent_delta > 0 else ""}{quiescence_percent_delta:.2f}%)")
        print(f"  % Looked Up ...... {f"{lookup_percent_avg:.2f}%":12}({"+" if lookup_percent_delta > 0 else ""}{lookup_percent_delta:.2f}%)")
        print(f"  Depth Completed .. {f"{depth_completed_avg:.2f}":12}({"+" if depth_completed_delta > 0 else ""}{depth_completed_delta:.2f}%)")
        print(f"  TT Occupancy ..... {f"{tt_occupancy_avg:.2f} MiB":12}({"+" if tt_occupancy_delta > 0 else ""}{tt_occupancy_delta:.2f}%)")
    else:
        print(f"  Search Speed ..... {search_speed_avg:.2f} Mnps")
        print(f"  % Quiesced ....... {quiescence_percent_avg:.2f}%")
        print(f"  % Looked Up ...... {lookup_percent_avg:.2f}%")
        print(f"  Depth Completed .. {depth_completed_avg:.2f}")
        print(f"  TT Occupancy ..... {tt_occupancy_avg:.2f} MiB")

    # Save averages if file provided
    if filename is not None:
        with open(filename, "w") as f:
            f.write(f"""
{{
    "search_speed": {search_speed_avg},
    "quiescence_percent": {quiescence_percent_avg},
    "lookup_percent": {lookup_percent_avg},
    "depth_completed": {depth_completed_avg},
    "tt_occupancy": {tt_occupancy_avg}
}}
""")

def progress_bar(n: int, max: int):
    WIDTH: int = 32
    progress: int = math.floor(n / max * WIDTH)
    bar: str = ""
    for i in range(WIDTH):
        if i <= progress:
            bar += "#"
        else:
            bar += "-"

    bar += f" ({n}/{max})"
    return bar

def main():
    out_filename: str | None = sys.argv[1] if len(sys.argv) >= 2 else None
    cmp_filename: str | None = sys.argv[2] if len(sys.argv) == 3 else None

    if cmp_filename is not None:
        try:
            f = open(cmp_filename, "r")
            f.close()
        except:
            print("Invalid comparison filename")
            return

    PLAY_DEPTH = 8

    with open("profiles/fen.txt", "r") as f:
        telemetry: list[str] = []
        metrics: list[str] = []
        lines: list[str] = f.readlines()
        n_processed: int = 0
        for i, line in enumerate(lines):
            fen: str = line.strip("\n \r")
            board: Board = Board(fen)
            board.set_time_control(300, 0)
            for j in range(PLAY_DEPTH):
                print(f"\rLoading: {progress_bar(i * PLAY_DEPTH + j, len(lines) * PLAY_DEPTH)}", end="")
                n_processed += 1
                move = board.go(60 * 1000)
                telemetry.append(board.get_telemetry())

                if move is None:
                    break
                board.make_move(move)

            metrics.append(board.get_metrics())
        print()
        process_results(out_filename, cmp_filename, telemetry, metrics)

if __name__ == "__main__":
    main()
