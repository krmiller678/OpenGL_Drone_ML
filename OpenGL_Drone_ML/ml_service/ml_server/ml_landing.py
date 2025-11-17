# ml_landing.py
import os, time
import numpy as np
import torch
import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
from pointnet_small import PointNetSeg

# --- Parameters ---
GRID_SIZE = 5
POINTS_PER_FRAME = GRID_SIZE * GRID_SIZE
MODEL_PATH = "safe_landing_model.pt"
SPACING = 25.0  # from handle_survey()
HALF = (GRID_SIZE - 1) / 2.0

# --- Load model once ---
device = torch.device("cuda" if torch.cuda.is_available() else "cpu")
model = PointNetSeg(num_classes=2).to(device)
model.load_state_dict(torch.load(MODEL_PATH, map_location=device))
model.eval()
print(f"✅ Loaded safe landing model on {device}")

# --- Coordinate conversion (matches handle_survey) ---
def lidar_to_world_points(pos, lidar):
    """Convert 5x5 lidar height map into 25x3 world-space points using survey offsets."""
    rows, cols = lidar.shape  # 5x5
    x_offsets = np.array([(j - HALF) * SPACING for j in range(cols)])
    z_offsets = np.array([(i - HALF) * SPACING for i in range(rows)])
    X, Z = np.meshgrid(x_offsets, z_offsets)
    X += pos["x"]
    Z += pos["z"]
    Y = lidar
    pts = np.stack([X.ravel(), Y.ravel(), Z.ravel()], axis=1)
    return pts.astype(np.float32)

# --- Local features (same as training) ---
def compute_local_features(frame_pts):
    y_grid = frame_pts[:, 1].reshape(GRID_SIZE, GRID_SIZE)
    mean_grid = np.zeros_like(y_grid)
    var_grid = np.zeros_like(y_grid)
    for i in range(GRID_SIZE):
        for j in range(GRID_SIZE):
            i0, i1 = max(0, i-1), min(GRID_SIZE, i+2)
            j0, j1 = max(0, j-1), min(GRID_SIZE, j+2)
            patch = y_grid[i0:i1, j0:j1]
            mean_grid[i, j] = patch.mean()
            var_grid[i, j] = patch.var()
    return np.stack([
        frame_pts[:, 0],
        frame_pts[:, 1],
        frame_pts[:, 2],
        mean_grid.flatten(),
        var_grid.flatten()
    ], axis=1)

# --- Visualization for debugging ---
def visualize_matplotlib(points, preds, landing_point, out_dir="./"):
    os.makedirs(out_dir, exist_ok=True)
    ts = int(time.time() * 1000)
    out_path = os.path.join(out_dir, f"landing_viz_{ts}.png")

    safe_mask = (preds == 1)
    xs, ys, zs = points[:, 0], points[:, 1], points[:, 2]
    plt.figure(figsize=(5,5))
    plt.title("ML Landing Inference (Top-Down View)")
    plt.scatter(xs[~safe_mask], zs[~safe_mask], c='red', s=30, label="unsafe")
    plt.scatter(xs[safe_mask], zs[safe_mask], c='green', s=30, label="safe")
    plt.scatter(landing_point[0], landing_point[2], c='blue', s=100, edgecolors='k', label="landing")
    plt.xlabel("X (world)")
    plt.ylabel("Z (world)")
    plt.legend()
    plt.tight_layout()
    plt.savefig(out_path, dpi=150)
    plt.close()
    return out_path

def save_points_open3D(lidar_below_drone, out_dir="./"):
    os.makedirs(out_dir, exist_ok=True)
    ts = int(time.time() * 1000)
    out_path = os.path.join(out_dir, f"live_LiDAR.pts") # live_LiDAR{ts}.pts

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
    return out_path

# --- Core inference entrypoint (used by helpers3d.find_best_landing) ---
def get_ml_landing_point(lidar_below_drone, visualize_flag=True):
    """
    lidar_below_drone: deque of up to 10 (pos, lidar_frame)
    Returns dict {"x":..., "y":..., "z":...} for best landing.
    """
    # save lidar_below_drone for outside visualization
    lidar_path = save_points_open3D(lidar_below_drone)
    print(f"✅ LiDAR terrain saved to {lidar_path}", flush=True)

    for frame_idx, (pos, lidar_frame) in enumerate(lidar_below_drone):
        points = lidar_to_world_points(pos, np.array(lidar_frame))
        frame_features = compute_local_features(points)
        pts_tensor = torch.tensor(frame_features, dtype=torch.float32).unsqueeze(0).to(device)

        with torch.no_grad():
            outputs = model(pts_tensor)  # [1, 25, 2]
            preds = torch.argmax(outputs, dim=2).cpu().numpy().flatten()

        if np.any(preds == 1):
            # Choose lowest-variance "safe" region
            safe_pts = points[preds == 1]
            variances = np.var(safe_pts[:, 1])
            landing_point = safe_pts[np.argmin(variances)]
            if visualize_flag:
                img_path = visualize_matplotlib(points, preds, landing_point)
                print(f"✅ Frame {frame_idx}: safe landing found, saved {img_path}", flush=True)
            return {
                "x": float(landing_point[0]),
                "y": float(landing_point[1]),
                "z": float(landing_point[2])
            }

    print("⚠️ No safe landing zones found in ML inference.")
    return None
