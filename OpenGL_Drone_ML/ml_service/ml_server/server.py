from flask import Flask, request, jsonify

app = Flask(__name__)

delta = 1

# Dummy ML model
def compute_next_action(state):
    global delta
    if state["y"] == 540 or state["y"] == 0:
        delta = -delta
    return {"x": state["x"], "y": state["y"] + delta}

@app.route("/compute", methods=["POST"])
def compute():
    state = request.json
    delta = 1
    result = compute_next_action(state)
    return jsonify(result)

if __name__ == "__main__":
    app.run(host="0.0.0.0", port=5000)