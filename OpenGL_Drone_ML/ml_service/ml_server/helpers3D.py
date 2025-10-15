import math
import time
from collections import deque
from python_tsp.heuristics import solve_tsp_local_search
import numpy as np


delta = 50
waiting = False
em_stop_pos_assigned = False
wait_start_time = 0
wait_duration = 2
em_stop_pos = 0

CRUISE_HEIGHT = 100  
PHASE_CRUISE = 0      
PHASE_DESCEND = 1     
PHASE_ASCEND = 2      
drone_phase = PHASE_CRUISE 

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
    dz = target["z"] - current["z"]
    dist = math.sqrt(dx*dx + dy*dy + dz*dz)

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
            "z": current["z"] + dz * ratio
        }
        return new_pos, False
    
def find_best_landing(lidar_below_drone, start_pos):
    """Looks at position coordinates and associated lidar scan to find best landing spot"""
    global em_stop_pos_assigned
    em_stop_pos_assigned = True
    for pos_lidar in lidar_below_drone:
        pos = pos_lidar[0]
        lidar_lists = pos_lidar[1]
        for i in range(4):
            for j in range(4):
                if (lidar_lists[i][j] - lidar_lists[i+1][j+1] <= 3 and 
                    lidar_lists[i][j] - lidar_lists[i+1][j+1] >= -3 and
                    lidar_lists[i][j] >= -20):
                    em_stop_pos = {
                        "x": pos["x"] + (i-1.5)*25,
                        "y": lidar_lists[i][j],
                        "z": pos["z"] + (j-1.5)*25
                    } # need to fix the y at some point 
                    return em_stop_pos
    return start_pos

def move_horizontal(current, target):
    """Move only in X and Z toward the target; Y stays constant."""
    dx = target["x"] - current["x"]
    dz = target["z"] - current["z"]
    dist = math.sqrt(dx*dx + dz*dz)
    
    if dist <= delta:
        return {"x": target["x"], "y": current["y"], "z": target["z"]}, True
    else:
        ratio = delta / dist
        new_pos = {
            "x": current["x"] + dx * ratio,
            "y": target["y"],
            "z": current["z"] + dz * ratio
        } # set y to target to climb over obstacles
        return new_pos, False
    
def descend_or_ascend(current, target_y):
    """Move only in Y toward target_y."""
    dy = target_y - current["y"]
    dist = abs(dy)
    
    if dist <= delta:
        return {"x": current["x"], "y": target_y, "z": current["z"]}, True
    else:
        step = delta if dy > 0 else -delta
        return {"x": current["x"], "y": current["y"] + step, "z": current["z"]}, False

    
def handle_3D_input(current, targets, start_pos, emergency_stop, lidar_below_drone):
    global em_stop_pos, em_stop_pos_assigned , drone_phase , waiting , wait_start_time
    if waiting:
        should_wait() # this should also be a safe landing for package pickup
        return current
    elif emergency_stop:
        if not em_stop_pos_assigned:
            em_stop_pos = find_best_landing(lidar_below_drone, start_pos)
        new_pos, reached = move_toward_target(current, em_stop_pos)
        return new_pos
    elif not targets:
        return start_pos
    else:
        em_stop_pos_assigned = False
        target = targets[0]

        if drone_phase == PHASE_CRUISE:  
            hover_target = {"x": target["x"], "y": max(CRUISE_HEIGHT + (lidar_below_drone[0][1][2][2] if lidar_below_drone else 0), CRUISE_HEIGHT), "z": target["z"]}  # added
            new_pos, reached = move_horizontal(current, hover_target)  
            if reached:  
                drone_phase = PHASE_DESCEND  
            return new_pos  

        elif drone_phase == PHASE_DESCEND:  
            new_pos, reached = descend_or_ascend(current, target["y"])  
            if reached:  
                waiting = True  
                wait_start_time = time.time()
                drone_phase = PHASE_ASCEND
                targets.pop(0)
            return new_pos  

        elif drone_phase == PHASE_ASCEND:  
            new_pos, reached = descend_or_ascend(current, CRUISE_HEIGHT)  
            if reached:  
                drone_phase = PHASE_CRUISE
            return new_pos