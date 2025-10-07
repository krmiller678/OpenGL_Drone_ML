import importlib.util
import sys
from pathlib import Path
import pytest
import math

helpers_path = Path(__file__).parent.parent / "helpers2D.py"
spec = importlib.util.spec_from_file_location("helpers2D", str(helpers_path))
helpers2D = importlib.util.module_from_spec(spec)
sys.modules["helpers2D"] = helpers2D
spec.loader.exec_module(helpers2D)

reorder_targets_shortest_cycle = helpers2D.reorder_targets_shortest_cycle

def make_point(x, y, z=0):
    return {"x": x, "y": y, "z": z}

def path_distance(points):
    total = 0
    for i in range(len(points) - 1):
        dx = points[i+1]["x"] - points[i]["x"]
        dy = points[i+1]["y"] - points[i]["y"]
        dz = points[i+1]["z"] - points[i]["z"]
        total += math.sqrt(dx**2 + dy**2 + dz**2)
    return total

# tests
def test_tsp_reduces_total_distance_simple_triangle():
    start = make_point(0, 0)
    targets = [
        make_point(100, 0),
        make_point(0, 100),
        make_point(50, 50)
    ]
    original_path = [start] + targets + [start]
    original_dist = path_distance(original_path)

    reordered = reorder_targets_shortest_cycle(targets, start)
    reordered_path = [start] + reordered
    reordered_dist = path_distance(reordered_path)

    assert reordered_dist < original_dist

def test_tsp_handles_line_points():
    start = make_point(0, 0)
    # already optimal 
    targets = [
        make_point(100, 0),
        make_point(200, 0),
        make_point(300, 0),
    ]
    original_path = [start] + targets + [start]
    original_dist = path_distance(original_path)

    reordered = reorder_targets_shortest_cycle(targets, start)
    reordered_path = [start] + reordered
    reordered_dist = path_distance(reordered_path)

    # Should be roughly equal since it's already optimal
    assert math.isclose(reordered_dist, original_dist, rel_tol=0.01)

def test_tsp_random_square_points():
    start = make_point(0, 0)
    # non-optimal order on purpose
    targets = [
        make_point(100, 100),
        make_point(0, 100),
        make_point(100, 0),
        make_point(50, 50),
    ]
    original_path = [start] + targets + [start]
    original_dist = path_distance(original_path)

    reordered = reorder_targets_shortest_cycle(targets, start)
    reordered_path = [start] + reordered
    reordered_dist = path_distance(reordered_path)

    assert reordered_dist < original_dist

def test_tsp_single_target():
    start = make_point(0, 0)
    targets = [make_point(100, 100)]
    original_path = [start] + targets + [start]
    original_dist = path_distance(original_path)

    reordered = reorder_targets_shortest_cycle(targets, start)
    reordered_path = [start] + reordered
    reordered_dist = path_distance(reordered_path)

    assert math.isclose(reordered_dist, original_dist, rel_tol=0.01)

def test_tsp_no_targets():
    start = make_point(0, 0)
    targets = []
    original_path = [start, start]
    original_dist = path_distance(original_path)

    reordered = reorder_targets_shortest_cycle(targets, start)
    reordered_path = [start] + reordered
    reordered_dist = path_distance(reordered_path)

    assert math.isclose(reordered_dist, original_dist, rel_tol=0.01)

    
# ----------------------- Run manually -----------------------
if __name__ == "__main__":
    pytest.main([__file__])