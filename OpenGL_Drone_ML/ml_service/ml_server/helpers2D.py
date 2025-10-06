import math
import time
from collections import deque
from python_tsp.heuristics import solve_tsp_local_search
import numpy as np


delta = 50
waiting = False
wait_start_time = 0
wait_duration = 2

def reorder_targets_shortest_cycle(targets, start_pos):
    """Reorder targets to minimize travel distance, starting and ending at start_pos."""
    all_points = [start_pos] + targets
    n = len(all_points)

    # Create distance matrix
    dist_matrix = np.zeros((n, n))
    for i in range(n):
        for j in range(n):
            if i != j:
                dx = all_points[i]["x"] - all_points[j]["x"]
                dy = all_points[i]["y"] - all_points[j]["y"]
                dz = all_points[i]["z"] - all_points[j]["z"]
                dist_matrix[i, j] = (dx**2 + dy**2 + dz**2) ** 0.5

    # tsp solve
    permutation, _ = solve_tsp_local_search(dist_matrix)

    # rotate output so we start at start_pos 
    start_idx = permutation.index(0)
    rotated_permutation = permutation[start_idx:] + permutation[:start_idx]

    # return reordered targets then tack on start pos at end
    reordered = [all_points[i] for i in rotated_permutation[1:]] + [start_pos]
    return reordered

def should_wait():
    """Check if we’re currently waiting at a target."""
    global waiting, wait_start_time
    if time.time() - wait_start_time >= wait_duration:
        waiting = False

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
    
def handle_2D_input(current, targets, start_pos, emergency_stop, lidar_below_drone):
    if waiting:
        should_wait() # this should also be a safe landing for package pickup
        return current
    elif emergency_stop or not targets:
        new_pos, reached = move_toward_target(current, start_pos)
        return new_pos
    else:
        new_pos, reached = move_toward_target(current, targets[0])
        if reached:
            if targets:
                targets.pop(0)
        return new_pos