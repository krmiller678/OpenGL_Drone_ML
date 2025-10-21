# ml_landing.py
import numpy as np
import torch
import open3d as o3d
from pointnet_small import PointNetSmall

# --- Load model globally ---
model = PointNetSmall()
model.load_state_dict(torch.load("safe_landing_model.pt", map_location="cpu"))
model.eval()

def frame_to_points(lidar_frame):
    y_vals = np.array(lidar_frame)
    grid_size = y_vals.shape[0]
    x, z = np.meshgrid(np.linspace(-1, 1, grid_size), np.linspace(-1, 1, grid_size))
    return np.stack([x, y_vals, z], axis=-1).reshape(-1, 3)

def find_safest_point(points, grid_size=5):
    y = points[:,1].reshape(grid_size, grid_size)
    var_map = np.zeros_like(y)
    for i in range(grid_size):
        for j in range(grid_size):
            i0, i1 = max(0, i-1), min(grid_size, i+2)
            j0, j1 = max(0, j-1), min(grid_size, j+2)
            var_map[i,j] = np.var(y[i0:i1,j0:j1])
    idx = np.unravel_index(np.argmin(var_map), var_map.shape)
    return points[idx[0]*grid_size + idx[1]]

def visualize(points, landing_point, is_safe=True):
    pcd = o3d.geometry.PointCloud()
    pcd.points = o3d.utility.Vector3dVector(points)
    color = np.array([[0,1,0] if is_safe else [1,0,0]] * len(points))
    pcd.colors = o3d.utility.Vector3dVector(color)
    sphere = o3d.geometry.TriangleMesh.create_sphere(radius=0.05)
    sphere.paint_uniform_color([0,0,1])
    sphere.translate(landing_point)
    o3d.visualization.draw_geometries([pcd, sphere], window_name="Emergency Landing Zone")

def get_ml_landing_point(lidar_below_drone, visualize_flag=True):
    pos, lidar_frame = lidar_below_drone[0]  # newest (leftmost)
    points = frame_to_points(lidar_frame)
    with torch.no_grad():
        preds = model(torch.tensor(points, dtype=torch.float32).unsqueeze(0))
        is_safe = torch.argmax(preds, dim=1).item() == 1
    landing_point = find_safest_point(points)
    if visualize_flag:
        visualize(points, landing_point, is_safe)
    return {
        "x": pos["x"] + landing_point[0]*50,
        "y": landing_point[1],
        "z": pos["z"] + landing_point[2]*50
    }
