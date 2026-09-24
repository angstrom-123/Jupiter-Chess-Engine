from functools import wraps

from jupiterengine.build import libjupiter

def not_none(attr_name: str):
    def decorator(func):
        @wraps(func)
        def wrapper(self, *args, **kwargs):
            if not hasattr(self, attr_name) or getattr(self, attr_name) is None:
                raise AttributeError(f"[JUPITER] required attribute '{attr_name}' is not initialised.")
            return func(self, *args, **kwargs)
        return wrapper 
    return decorator


class Range():
    def __init__(self, min: float, max: float):
        self.min: float = min 
        self.max: float = max 

    def range(self) -> float:
        return self.max - self.min

    def __neg__(self) -> "Range":
        return Range(-self.max, -self.min)

SPSA_WEIGHT_RANGE: Range = Range(0.0, 2.0)
SPSA_SMALL_RANGE: Range = Range(0.0, 25.0)
SPSA_LARGE_RANGE: Range = Range(0.0, 100.0)

# These are hardcoded ranges for each evaluation constant for use in normalising 
# because they all have different scales. These are just guesses and are by no 
# means perfect, but they let me normalise my params for algorithmic tuning.
SPSA_RANGES: dict[str, Range] = {
    # These are weight multipliers so keep them small
    "material_weight": SPSA_WEIGHT_RANGE,
    "pst_weight": SPSA_WEIGHT_RANGE,

    # These are small factors so medium-small range
    "mopup_proximity_factor": SPSA_SMALL_RANGE,
    "mopup_edge_factor": SPSA_SMALL_RANGE,
    "mobility_factor": SPSA_SMALL_RANGE,
    "king_pawn_tropism_regular_factor": SPSA_SMALL_RANGE,
    "king_pawn_tropism_weak_factor": SPSA_SMALL_RANGE,
    "king_pawn_tropism_passed_factor": SPSA_SMALL_RANGE,
    "storming_pawn_factor": -SPSA_SMALL_RANGE, # Penalty
    "connected_pawn_factor": SPSA_SMALL_RANGE,

    # These are larger factors so allow larger ranges
    "king_mobility_factor": -SPSA_LARGE_RANGE, # Penalty
    "missing_shield_pawn_factor": -SPSA_LARGE_RANGE, # Penalty
    "weak_pawn_factor": -SPSA_LARGE_RANGE, # Penalty
    "slider_open_file_factor": SPSA_LARGE_RANGE,
}

# For tuning, Jupiter Interface uses SPSA which works best with scaled values. 
# Evaluation weights are not uniform so are all individually scaled based on their 
# expected upper and lower bounds (into a -1.0 to 1.0 range that is needed by SPSA).
# The functions below are the helpers for this, the weight table is declared above.

# Puts all evaluation factors in a -1.0 to 1.0 range for SPSA
def normalise_weights(weights: dict[str, int | float]) -> dict[str, float]:
    res: dict[str, float] = {}
    for k, v in weights.items():
        res[k] = float(v) / SPSA_RANGES[k].range()
    return res

# Converts weights back from -1.0 to 1.0 range to scaled values
def denormalise_weights(weights: dict[str, float]) -> dict[str, float]:
    res: dict[str, int | float] = {}
    for k, v in weights.items():
        res[k] = float(v * SPSA_RANGES[k].range())
    return res

def assign_weights(board: libjupiter.Board, weights: dict[str, float]) -> None:
    board.set_weights(
        material_weight=weights["material_weight"],
        pst_weight=weights["pst_weight"],
        mopup_proximity_factor=round(weights["mopup_proximity_factor"]),
        mopup_edge_factor=round(weights["mopup_edge_factor"]),
        king_mobility_factor=round(weights["king_mobility_factor"]),
        mobility_factor=round(weights["mobility_factor"]),
        king_pawn_tropism_regular_factor=round(weights["king_pawn_tropism_regular_factor"]),
        king_pawn_tropism_weak_factor=round(weights["king_pawn_tropism_weak_factor"]),
        king_pawn_tropism_passed_factor=round(weights["king_pawn_tropism_passed_factor"]),
        missing_shield_pawn_factor=round(weights["missing_shield_pawn_factor"]),
        storming_pawn_factor=round(weights["storming_pawn_factor"]),
        slider_open_file_factor=round(weights["slider_open_file_factor"]),
        weak_pawn_factor=round(weights["weak_pawn_factor"]),
        connected_pawn_factor=round(weights["connected_pawn_factor"])
    )

def print_telemetry_and_metrics(telemetry: dict[str, int], metrics: dict[str, int], n_searches: int) -> None:
    if telemetry["searchTime"] == 0:
        print("[JUPITER] No metrics to show - never left opening book")
        return

    print(f"""
[JUPITER] Game metrics:
- avg Nodes Searched  : {(telemetry["nodesSearched"] / n_searches) / 1_000_000:.3f}M
- avg Nodes Quiesced  : {(telemetry["nodesQuiesced"] / n_searches) / 1_000_000:.3f}M
- avg Search Speed    : {telemetry["nodesSearched"] / (telemetry["searchTime"] * 1000):.3f}mnps 
- avg Quiescence %    : {(telemetry["nodesQuiesced"] / telemetry["nodesSearched"]) * 100:.3f}%
- avg Completed Depth : {telemetry["depth"] / n_searches:.3f}
- avg Lookup %        : {(telemetry["nodesLookedUp"] / telemetry["nodesSearched"]) * 100:.3f}%
- avg Pawn Lookup %   : {(telemetry["pawnsLookedUp"] / telemetry["evaluations"]) * 100:.3f}%
- TT Occupancy        : {metrics["ttSize"] / (1024 * 1024):.3f}MiB
- PT Occupancy        : {metrics["ptSize"] / (1024 * 1024):.3f}MiB
- Book Moves          : {metrics["bookMoves"]}
- Searches Completed  : {n_searches}
""")
