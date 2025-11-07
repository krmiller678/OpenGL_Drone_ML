import math
import time
from collections import deque
import numpy as np


delta = 50

CRUISE_HEIGHT = 200

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
            "y": CRUISE_HEIGHT,
            "z": current["z"] + dz * ratio
        } # set y to target to climb over obstacles
        return new_pos, False

    
def handle_survey(current, targets, start_pos, emergency_stop, lidar_below_drone):
    if not targets:
        return start_pos
    else:
        target = targets[0]
        new_pos, reached = move_horizontal(current, target)
        if reached:  
            targets.pop(0) 
        return new_pos