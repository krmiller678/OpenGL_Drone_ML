import torch
import numpy as np
import open3d as o3d
from pointnet_small import PointNetSeg
import random
import os

# ----- Parameters -----
TRAIN_DIR = "./"
MODEL_PATH = "safe_landing_model.pt"
POINTS_PER_FRAME = 25  # 5x5
NUM_FRAMES = 10
GRID_SIZE = 5

# ----- Load Random File -----
pts_files = [f for f in os.listdir(TRAIN_DIR) if f.endswith(".pts")]
file_path = os.path.join(TRAIN_DIR, "live_LiDAR.pts") #random.choice(pts_files)
points = np.loadtxt(file_path, dtype=np.float32)  # [250, 3]

# ----- Load Model -----
device = torch.device("cuda" if torch.cuda.is_available() else "cpu")
model = PointNetSeg(num_classes=2).to(device)
model.load_state_dict(torch.load(MODEL_PATH, map_location=device))
model.eval()

# ----- Compute extra features per point (local mean + variance) -----
all_features = []

for f in range(NUM_FRAMES):
    frame_pts = points[f*POINTS_PER_FRAME:(f+1)*POINTS_PER_FRAME]  # [25,3]
    y_grid = frame_pts[:,1].reshape(5,5)

    mean_grid = np.zeros_like(y_grid)
    var_grid  = np.zeros_like(y_grid)
    for i in range(5):
        for j in range(5):
            i_min, i_max = max(0,i-1), min(5,i+2)
            j_min, j_max = max(0,j-1), min(5,j+2)
            patch = y_grid[i_min:i_max, j_min:j_max]
            mean_grid[i,j] = patch.mean()
            var_grid[i,j]  = patch.var()

    mean_flat = mean_grid.flatten()
    var_flat  = var_grid.flatten()
    frame_features = np.stack([
        frame_pts[:,0],  # x
        frame_pts[:,1],  # y
        frame_pts[:,2],  # z
        mean_flat,       # local mean
        var_flat         # local variance
    ], axis=1)           # [25,5]
    all_features.append(frame_features)

# Stack all frames -> [250,5]
pts_features = np.vstack(all_features)
pts_tensor = torch.tensor(pts_features, dtype=torch.float32).unsqueeze(0).to(device)  # [1,250,5]

# ----- Run Inference -----
with torch.no_grad():
    outputs = model(pts_tensor)  # [1, 250, 2]
    preds = torch.argmax(outputs, dim=2).cpu().numpy().flatten()  # [250]

# ----- Point Cloud -----
pcd = o3d.geometry.PointCloud()
pcd.points = o3d.utility.Vector3dVector(points)

# Color points by label
colors = np.zeros((points.shape[0], 3))
colors[preds == 1] = [0, 1, 0]  # green = safe
colors[preds == 0] = [1, 0, 0]  # red = unsafe
pcd.colors = o3d.utility.Vector3dVector(colors)

# ----- Create surface meshes per frame -----
meshes = []
num_frames = points.shape[0] // POINTS_PER_FRAME

for f in range(num_frames):
    frame_pts = points[f*POINTS_PER_FRAME:(f+1)*POINTS_PER_FRAME].reshape(GRID_SIZE, GRID_SIZE, 3)
    
    vertices = frame_pts.reshape(-1, 3)
    triangles = []

    # Create two triangles per quad
    for i in range(GRID_SIZE-1):
        for j in range(GRID_SIZE-1):
            idx0 = i*GRID_SIZE + j
            idx1 = i*GRID_SIZE + (j+1)
            idx2 = (i+1)*GRID_SIZE + j
            idx3 = (i+1)*GRID_SIZE + (j+1)

            triangles.append([idx0, idx2, idx1])
            triangles.append([idx1, idx2, idx3])

    mesh = o3d.geometry.TriangleMesh()
    mesh.vertices = o3d.utility.Vector3dVector(vertices)
    mesh.triangles = o3d.utility.Vector3iVector(triangles)

    # Color mesh according to predicted labels
    mesh_colors = np.zeros((vertices.shape[0], 3))
    mesh_colors[preds[f*POINTS_PER_FRAME:(f+1)*POINTS_PER_FRAME] == 1] = [0, 1, 0]
    mesh_colors[preds[f*POINTS_PER_FRAME:(f+1)*POINTS_PER_FRAME] == 0] = [1, 0, 0]
    mesh.vertex_colors = o3d.utility.Vector3dVector(mesh_colors)

    meshes.append(mesh)

# ----- Highlight a special point (lowest X, highest Z) -----
special_idx = np.argmin(points[:,0] + (-points[:,2]))  # lowest X + highest Z
sphere = o3d.geometry.TriangleMesh.create_sphere(radius=2.0)
sphere.translate(points[special_idx])
sphere.paint_uniform_color([1,1,0])  # yellow

# ----- Visualizer options -----
vis = o3d.visualization.Visualizer()
vis.create_window(window_name="Safe Landing Zones Surface", width=1280, height=720)
opt = vis.get_render_option()
opt.background_color = np.array([0.05, 0.05, 0.05])  # darker background
opt.point_size = 5.0
opt.show_coordinate_frame = False

# Add geometries
vis.add_geometry(pcd)
for mesh in meshes:
    vis.add_geometry(mesh)
vis.add_geometry(sphere)

vis.run()
vis.destroy_window()
