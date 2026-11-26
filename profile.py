#!/usr/bin/env python3

from jupiterengine.build.libjupiter import Board 

import time
import json

board = Board("r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1")

time.sleep(1)

board.go(100_000)
telem = json.loads(board.get_telemetry())
n = telem["nodesSearched"]
t = telem["searchTime"]
print(f"Searched {n / 1_000_000:.3f}M nodes in {t}s ({n / (t * 1000.0):.3f}mnps")
