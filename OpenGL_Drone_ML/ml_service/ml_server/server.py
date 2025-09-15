from flask import Flask, request, jsonify
import math
import time

app = Flask(__name__)

delta = 5  # step size
visited_targets = []
waiting = False
wait_start_time = 0
wait_duration = 3  # seconds to wait at each target

def distance(p1, p2):
    """Euclidean distance in 2D."""
    return math.sqrt((p1["x"] - p2["x"])**2 + (p1["y"] - p2["y"])**2)

@app.route("/compute", methods=["POST"])
def compute():
    global visited_targets, waiting, wait_start_time, delta

    state = request.json
    test_name = state.get("test", "")

    # ---------- LEFT-RIGHT LOGIC FOR OTHER TESTS ----------
    if test_name != "CooperTest" and test_name != "Test2DMultiTexture":
        current = state["current"]
        if current["x"] >= 960 or current["x"] <= 0:
            delta = -delta
        return jsonify({"x": current["x"] + delta, "y": current["y"], "z": current["z"]})

    # ---------- COOPERTEST PATH-FOLLOWING LOGIC ----------
    current = state["current"]
    targets = state["targets"]
    start = state["start"]

    # Initialize visited_targets if empty
    if not visited_targets:
        visited_targets = [False] * len(targets)

    # Waiting at a target
    if waiting:
        if time.time() - wait_start_time >= wait_duration:
            waiting = False  # done waiting, move to next
        else:
            return jsonify(current)  # stay in place

    # Find closest unvisited target
    closest_idx = None
    min_dist = float('inf')
    for i, t in enumerate(targets):
        if visited_targets[i]:
            continue
        d = distance(current, t)
        if d < min_dist:
            min_dist = d
            closest_idx = i

    # All targets visited -> go back to start
    if closest_idx is None:
        closest_idx = -1  # special index for start

    target = start if closest_idx == -1 else targets[closest_idx]

    # Move in straight line toward target by delta
    dx = target["x"] - current["x"]
    dy = target["y"] - current["y"]
    dist = math.sqrt(dx*dx + dy*dy)
    if dist <= delta:
        # Reached target
        new_pos = {"x": target["x"], "y": target["y"], "z": target["z"]}
        if closest_idx >= 0:
            visited_targets[closest_idx] = True
        else:
            visited_targets = [False] * len(targets)  # reset loop
        waiting = True
        wait_start_time = time.time()
    else:
        ratio = delta / dist
        new_pos = {"x": current["x"] + dx * ratio, "y": current["y"] + dy * ratio, "z": current["z"]}

    return jsonify(new_pos)

if __name__ == "__main__":
    app.run(host="0.0.0.0", port=5000)
