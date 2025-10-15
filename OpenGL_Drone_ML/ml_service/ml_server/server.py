"""
This module is our primary server that takes in sensor information from
the simulation and responds with commands for the actuators (motion).
There are 3 stages for handling sensorics and dispatching commands:
1. Initialization Stage
    - Takes in JSON {
            "current":{"x":200.0,"y":200.0,"z":1.0},
            "emergency_stop":false,
            "targets":[
                {"x":733.0,"y":295.0,"z":0.0},
                {"x":522.0,"y":191.0,"z":0.0}
            ],
            "test":"Test2DMultiTexture"
        }
2. Control Stage
    - Takes in JSON {
            "current":{"x":200.0,"y":200.0,"z":0.0},
            "emergency_stop":false,
            "lidar_below_drone":[
                [0.8999999761581421, 0.8999999761581421, 0.0, 0.0, 0.0], 
                [0.8999999761581421, 0.8999999761581421, 0.0, 0.0, 0.0], 
                [0.0, 0.0, 0.0, 0.0, 0.0], 
                [0.0, 0.0, 0.0, 0.0, 0.0], 
                [0.0, 0.0, 0.0, 0.0, 0.0]
            ]
        }
    - Sends actuator commands
    - Looks to deploy emergency landing function
3. Emergency Landing Stage
    - Searches deque of potential landing positions (lidar_below_drone)
    - Sends actuator commands to land drone

"""

from flask import Flask, request, jsonify
from collections import deque
from helpersBasic import left_right
from helpers2D import handle_2D_input , reorder_targets_shortest_cycle
from helpers3D import handle_3D_input
app = Flask(__name__)

# --- GLOBALS ---
test_type = None
targets = []
start_pos = None
lidar_below_drone = deque(maxlen=10)
emergency_stop = False

# ---------- MAIN ROUTE ----------
@app.route("/compute", methods=["POST"])
def compute():
    global test_type, targets, start_pos, lidar_below_drone, emergency_stop

    try:
        state = request.json
    except Exception as e:
        return jsonify({"error": f"Invalid JSON: {str(e)}"}), 400
    
    test_name = state.get("test", test_type)
    current = state.get("current")
    if not current:
        return jsonify({"error": "'current' field missing"}), 400

    # Reset globals in event that test_name no longer matches test_type
    if test_name != test_type:
        test_type = test_name
        targets = [dict(t) for t in state["targets"]]
        start_pos = current
        if lidar_below_drone:
            lidar_below_drone.clear()
        emergency_stop = state.get("emergency_stop", emergency_stop)


        print("Before reordering:", targets, flush = True)
        targets = reorder_targets_shortest_cycle(targets, start_pos)
        print("After reordering:", targets, flush = True)

    if test_name == "Basic":
        return jsonify(left_right(current))
    elif test_name == "2DCT" or test_name == "2DMT":
        lidar_below_drone.appendleft([current, state.get("lidar_below_drone", lidar_below_drone)]) 
        print(lidar_below_drone, flush = True)
        emergency_stop = state.get("emergency_stop", emergency_stop)
        response = handle_2D_input(current, targets, start_pos, emergency_stop, lidar_below_drone)
        return jsonify(response)
    elif test_name == "3DA" or "3DB":
        lidar_below_drone.appendleft([current, state.get("lidar_below_drone", lidar_below_drone)]) 
        print(lidar_below_drone, flush = True)
        emergency_stop = state.get("emergency_stop", emergency_stop)
        response = handle_3D_input(current, targets, start_pos, emergency_stop, lidar_below_drone)
        return jsonify(response)
    else:
        return jsonify({"error": "No test type specified"}), 400

if __name__ == "__main__":
    app.run(host="0.0.0.0", port=5000, debug=True)
