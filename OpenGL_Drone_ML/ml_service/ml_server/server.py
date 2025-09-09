from flask import Flask, request, jsonify

app = Flask(__name__)

delta = 1

# Dummy ML model
def compute_next_action(state):
    global delta
    if state["x"] == 960 or state["x"] == 0:
        delta = -delta
    return {"x": state["x"] + delta, "y": state["y"] , "z": state["z"]}

@app.route("/compute", methods=["POST"])
def compute():
    state = request.json
    result = compute_next_action(state)
    return jsonify(result)

if __name__ == "__main__":
    app.run(host="0.0.0.0", port=5000)