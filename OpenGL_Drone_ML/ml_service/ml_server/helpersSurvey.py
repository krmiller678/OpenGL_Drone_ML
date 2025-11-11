# helpersSurvey

import math
import time
from collections import deque
import os
import json
from datetime import datetime, UTC
import copy
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
    # Save snapshot when deque is full (and not in emergency)
    try:
        if (
            targets and (not emergency_stop) 
            and lidar_below_drone is not None 
            and len(lidar_below_drone) == getattr(lidar_below_drone, "maxlen", None)
        ):
            # Create output directory
            os.makedirs("training_samples", exist_ok=True)

            # Make unique timestamp to save pointcloud
            ts = datetime.now(UTC).strftime("%Y%m%dT%H%M%S.%f")
            out_path = os.path.join("training_samples", f"sample_{ts}.pts")

            # Combine all lidar frames into a single 3D point cloud
            points = []
            for frame in list(lidar_below_drone):
                pos = frame[0] # this is the {x, y, z} position
                lidar = np.array(frame[1]) # y-values sampled around the point

                # interpret lidar points
                rows, cols = lidar.shape  # should both be 5

                spacing = 25.0
                half = (cols - 1) / 2.0  # 2.0 for 5x5
                x_offsets = np.array([(j - half) * spacing for j in range(cols)])
                z_offsets = np.array([(i - half) * spacing for i in range(rows)])

                # Create world coordinates
                X, Z = np.meshgrid(x_offsets, z_offsets)
                X += pos["x"]
                Z += pos["z"]
                Y = lidar  # the ground hit heights

                # Flatten to N×3
                pts = np.stack([X.ravel(), Y.ravel(), Z.ravel()], axis=1)
                points.append(pts)
            
            all_points = np.concatenate(points, axis=0)
            np.savetxt(out_path, all_points, fmt="%.4f")
            print(f"[handle_survey] Saved {len(all_points)} points -> {out_path}", flush=True)

    except Exception as e:
        # don't crash survey if saving fails; log or print for debugging
        print(f"[handle_survey] failed to save pointcloud: {e}", flush=True)

    if not targets:
        return start_pos
    else:
        target = targets[0]
        new_pos, reached = move_horizontal(current, target)
        if reached:  
            targets.pop(0) 
        return new_pos