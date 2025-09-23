from flask import Flask, request, jsonify
import math
import time
from helpers import distance
from left_right import left_right

app = Flask(__name__)

# --- GLOBALS ---
last_test_name = None
delta = 10  
remaining_targets = []   
all_targets = []         
start_pos = None
waiting = False
wait_start_time = 0
wait_duration = 2 

#stuff to move to helper file later

def should_wait():
    """Check if we’re currently waiting at a target."""
    global waiting, wait_start_time
    if waiting:
        if time.time() - wait_start_time >= wait_duration:
            waiting = False
            return False
        return True
    return False

def move_toward_target(current, target):
    """Compute next position moving toward a target by delta."""
    global waiting, wait_start_time
    dx = target["x"] - current["x"]
    dy = target["y"] - current["y"]
    dist = math.sqrt(dx*dx + dy*dy)

    if dist <= delta:
        # Reached target
        new_pos = {"x": target["x"], "y": target["y"], "z": target["z"]}
        waiting = True
        wait_start_time = time.time()
        return new_pos, True
    else:
        ratio = delta / dist
        new_pos = {
            "x": current["x"] + dx * ratio,
            "y": current["y"] + dy * ratio,
            "z": current["z"]
        }
        return new_pos, False

# ---------- MAIN ROUTE ----------
@app.route("/compute", methods=["POST"])
def compute():
    global remaining_targets, all_targets, start_pos, last_test_name

    try:
        state = request.json
    except Exception as e:
        return jsonify({"error": f"Invalid JSON: {str(e)}"}), 400

    current = state.get("current")
    if not current:
        return jsonify({"error": "'current' field missing"}), 400

    test_name = state.get("test" , last_test_name)
    emergency_stop = state.get("emergency_stop", False)

    # Reset on new test
    if test_name and test_name != last_test_name:
        remaining_targets.clear()
        all_targets.clear()
        start_pos = None
        last_test_name = test_name

    # Handle emergency stop
    if emergency_stop:
        new_pos = {"x": current["x"], "y": current["y"], "z": current["z"] + 200}
        return jsonify(new_pos)

    # LEFT-RIGHT logic for other tests
    if test_name not in ("CooperTest", "Test2DMultiTexture"):
        return jsonify(left_right(current))

    # Initialize targets/start on first full payload
    if state.get("targets") and state.get("start") and not remaining_targets:
        all_targets = [dict(t) for t in state["targets"]]
        remaining_targets = [dict(t) for t in state["targets"]]
        start_pos = dict(state["start"])

    if should_wait():
        return jsonify(current)

    # Determine next target
    if remaining_targets:
        target = remaining_targets[0]
    else:
        # all visited → reset loop
        remaining_targets = [dict(t) for t in all_targets]
        target = remaining_targets[0]

    new_pos, reached = move_toward_target(current, target)

    if reached:
        # remove target from remaining
        if remaining_targets:
            remaining_targets.pop(0)

    return jsonify(new_pos)

if __name__ == "__main__":
    app.run(host="0.0.0.0", port=5000, debug=True)
