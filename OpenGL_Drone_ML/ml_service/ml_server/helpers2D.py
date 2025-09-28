import math
import time
from collections import deque

delta = 10
waiting = False
wait_start_time = 0
wait_duration = 2

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