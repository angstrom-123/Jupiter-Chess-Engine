#!/usr/bin/env python3

import json
import subprocess

# Windows doesn't include readline in the stl
try:
    # Used to enable better navigation in the input prompt
    import readline
except ImportError:
    pass

from jupiterengine.build.libjupiter import Board

class Command:
    def __init__(self, name, proc):
        self.name = name
        self.proc = proc

commands: list[Command] = []
board: Board = Board()

def help_command(_):
    print("""
Commands:
    help
    quit
    show
    position [fen | 'startpos' | 'kiwipete']
    moves [lan] ...
    go [ms]
    profile
    perft [depth]
    metrics
""")

def quit_command(_):
    exit(0)

def show_command(_):
    print(repr(board))

def position_command(args):
    global board
    try:
        fen: str = ""
        if args[0] == "startpos":
            fen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"
        elif args[0] == "kiwipete":
            fen = "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1"
        elif len(args) == 6:
            fen = f"{args[0]} {args[1]} {args[2]} {args[3]} {args[4]} {args[5]}"
        else:
            raise ValueError()
        board = Board(fen)
    except:
        print("[ERROR] Bad args to position command.")

def moves_command(args):
    global board 
    try:
        for lan in args:
            board.make_move(lan)
    except:
        print("[ERROR] Bad args to moves command, board corrupted.")

def go_command(args):
    global board 
    try:
        lan = board.go(int(args[0]))
        if lan is None:
            print("[RESULT] No moves")
        else:
            telem = json.loads(board.get_telemetry()) # Not working?
            print(f"[RESULT] {lan}")

            n = telem["nodesSearched"]
            if n > 0:
                t = telem["searchTime"]
                d = telem["depth"]

                print(f"[INFO] Searched {n / 1_000_000:.3f}M nodes in {t}s ({n / (t * 1000.0):.3f}mnps) reaching depth {int(d)}")
            else:
                print("[INFO] No search performed; in opening book.")
    except:
        print("[ERROR] Bad args to go command.")

def profile_command(_):
    print("[WARNING] Local board is not used for profiling.")
    print("[WARNING] Not working on Windows.")
    try:
        subprocess.run("sudo perf record -F 20000 -g --call-graph dwarf -e cycles:pp --delay=1000 -- python3 profile.py && sudo perf report --dso=libjupiter.so", shell=True)
    except:
        print("[ERROR] Failed to run profile command.")

def perft_command(args):
    global board 
    try:
        n, div = board.perft_divide(int(args[0]))
        for k, v in div.items():
            print(f"  {k}: {v}")
        print(f"[RESULT] {n}")
    except Exception as e:
        print(e)
        print("[ERROR] Bad args to perft command.")

def metrics_command(_):
    try:
        m = board.get_metrics()
        for k, v in m:
            print(f"{k}: {v}")
    except:
        print("[ERROR] Failed to run metrics command.")

def init_commands():
    commands.append(Command("help",     help_command))
    commands.append(Command("quit",     quit_command))
    commands.append(Command("show",     show_command))
    commands.append(Command("position", position_command))
    commands.append(Command("moves",    moves_command))
    commands.append(Command("go",       go_command))
    commands.append(Command("profile",  profile_command))
    commands.append(Command("perft",    perft_command))
    commands.append(Command("metrics",  metrics_command))

def do_command(command):
    words = command.split(" ")
    for cmd in commands:
        if cmd.name == words[0]:
            cmd.proc(words[1:])

def main():
    init_commands()
    while 1:
        do_command(str(input("> ")))

if __name__ == "__main__":
    main()
