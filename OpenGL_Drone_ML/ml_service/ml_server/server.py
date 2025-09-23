from flask import Flask, request, jsonify
import math
import time
from helpers import distance
from left_right import left_right

app = Flask(__name__)

# --- GLOBALS ---
last_test_name = None
delta = 5  # step size
visited_targets = []
waiting = False
wait_start_time = 0
wait_duration = 3  # seconds to wait at each target




def init_visited_targets_if_needed(targets):
    """Initialize visited_targets list if it is empty."""
    global visited_targets
    if not visited_targets:
        visited_targets = [False] * len(targets)


def should_wait():
    """Check if we’re currently waiting at a target."""
    global waiting, wait_start_time
    if waiting:
        if time.time() - wait_start_time >= wait_duration:
            # Done waiting
            waiting = False
            return False
        else:
            return True
    return False


def find_closest_unvisited_target(current, targets):
    """Return the index of the closest unvisited target, or None if all visited."""
    min_dist = float('inf')
    closest_idx = None
    for i, t in enumerate(targets):
        if visited_targets[i]:
            continue
        d = distance(current, t)
        if d < min_dist:
            min_dist = d
            closest_idx = i
    return closest_idx


def move_toward_target(current, target):
    """Compute next position moving toward a target by delta."""
    global visited_targets, waiting, wait_start_time
    dx = target["x"] - current["x"]
    dy = target["y"] - current["y"]
    dist = math.sqrt(dx*dx + dy*dy)

    if dist <= delta:
        # Reached target
        new_pos = {"x": target["x"], "y": target["y"], "z": target["z"]}
        waiting = True
        wait_start_time = time.time()
    else:
        ratio = delta / dist
        new_pos = {
            "x": current["x"] + dx * ratio,
            "y": current["y"] + dy * ratio,
            "z": current["z"]
        }
    return new_pos, dist <= delta


def update_visited_or_reset(closest_idx, targets):
    """Update visited targets or reset if returning to start."""
    global visited_targets
    if closest_idx >= 0:
        visited_targets[closest_idx] = True
    else:
        visited_targets = [False] * len(targets)  # reset loop


# ---------- MAIN ROUTE ----------
@app.route("/compute", methods=["POST"])
def compute():
    global visited_targets , last_test_name

    state = request.json
    test_name = state.get("test", "")
    if not test_name:
        test_name = "hi"
    current = state["current"]

    if test_name != last_test_name:
        visited_targets.clear()
        last_test_name = test_name

    emergency_stop = state.get("emergency_stop", False)
    if emergency_stop:
        new_pos = {
            "x": current["x"] ,
            "y": current["y"] ,
            "z": current["z"] + 200
        }

        return jsonify(new_pos)


    # ---------- LEFT-RIGHT LOGIC FOR OTHER TESTS ----------
    if test_name not in ("CooperTest", "Test2DMultiTexture"):
        return jsonify(left_right(current))

    # ---------- COOPERTEST PATH-FOLLOWING LOGIC ----------
    targets = state["targets"]
    start = state["start"]

    init_visited_targets_if_needed(targets)

    if should_wait():
        return jsonify(current)  # stay in place while waiting

    closest_idx = find_closest_unvisited_target(current, targets)

    # All targets visited -> go back to start
    target = start if closest_idx is None else targets[closest_idx]

    new_pos, reached = move_toward_target(current, target)
    if reached:
        update_visited_or_reset(-1 if closest_idx is None else closest_idx, targets)

    return jsonify(new_pos)


if __name__ == "__main__":
    app.run(host="0.0.0.0", port=5000, debug=True)
