import torch
import numpy as np
import open3d as o3d
from pointnet_small import PointNetSeg
import random
import os

# ----- Parameters -----
TRAIN_DIR = "training_samples"
MODEL_PATH = "safe_landing_model.pt"
POINTS_PER_FRAME = 25  # 5x5
GRID_SIZE = 5

# ----- Load Random File -----
pts_files = [f for f in os.listdir(TRAIN_DIR) if f.endswith(".pts")]
file_path = os.path.join(TRAIN_DIR, "sample_20251111T175023.281959.pts") #random.choice(pts_files)
points = np.loadtxt(file_path, dtype=np.float32)  # [250, 3]

# ----- Load Model -----
device = torch.device("cuda" if torch.cuda.is_available() else "cpu")
model = PointNetSeg(num_classes=2).to(device)
model.load_state_dict(torch.load(MODEL_PATH, map_location=device))
model.eval()

# ----- Run Inference -----
pts_tensor = torch.tensor(points, dtype=torch.float32).unsqueeze(0).to(device)  # [1, 250, 3]
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
