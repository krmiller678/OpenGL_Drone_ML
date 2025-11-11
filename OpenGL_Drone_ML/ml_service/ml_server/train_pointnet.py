# train_pointnet.py
import os
import torch
import torch.nn as nn
import torch.optim as optim
from torch.utils.data import Dataset, DataLoader
import numpy as np
from pointnet_small import PointNetSeg

# ----- Parameters -----
TRAIN_DIR = "training_samples"   # directory with .pts files
VAR_THRESHOLD = 5.0              # variance threshold to label "safe" (1)
BATCH_SIZE = 16
EPOCHS = 30
LR = 0.001

# ----- Dataset -----
class LidarSegDataset(Dataset):
    def __init__(self, data_dir=TRAIN_DIR):
        self.files = [os.path.join(data_dir, f) for f in os.listdir(data_dir) if f.endswith(".pts")]

    def __len__(self):
        return len(self.files)

    def __getitem__(self, idx):
        pts = np.loadtxt(self.files[idx], dtype=np.float32)  # [250, 3]

        points_per_frame = 25
        num_frames = 10
        if pts.shape[0] != points_per_frame * num_frames:
            raise ValueError(f"Expected 250 points, got {pts.shape[0]}")

        labels = []
        for f in range(num_frames):
            frame_pts = pts[f*points_per_frame:(f+1)*points_per_frame]
            y_grid = frame_pts[:,1].reshape(5,5)
            labels_grid = np.zeros_like(y_grid, dtype=np.int64)

            for i in range(5):
                for j in range(5):
                    # 3x3 local neighborhood centered at (i, j)
                    i_min = max(0, i - 1)
                    i_max = min(5, i + 2)
                    j_min = max(0, j - 1)
                    j_max = min(5, j + 2)

                    local_patch = y_grid[i_min:i_max, j_min:j_max]
                    var = np.var(local_patch)
                    labels_grid[i,j] = 1 if var < VAR_THRESHOLD else 0

            labels.extend(labels_grid.flatten())

        return torch.tensor(pts, dtype=torch.float32), torch.tensor(labels, dtype=torch.long)

# ----- Load Dataset -----
dataset = LidarSegDataset()
train_loader = DataLoader(dataset, batch_size=BATCH_SIZE, shuffle=True)

# ----- Model, Loss, Optimizer -----
device = torch.device("cuda" if torch.cuda.is_available() else "cpu")
model = PointNetSeg(num_classes=2).to(device)
criterion = nn.CrossEntropyLoss()
optimizer = optim.Adam(model.parameters(), lr=LR)

# ----- Training Loop -----
for epoch in range(EPOCHS):
    model.train()
    total_loss = 0
    for pts, labels in train_loader:
        pts = pts.to(device)          # [B, 250, 3]
        labels = labels.to(device)    # [B, 250]

        optimizer.zero_grad()
        outputs = model(pts)          # [B, 250, 2]
        outputs = outputs.view(-1, 2)   # flatten for CE
        labels = labels.view(-1)
        loss = criterion(outputs, labels)
        loss.backward()
        optimizer.step()
        total_loss += loss.item()

    print(f"Epoch {epoch+1}/{EPOCHS} | Loss: {total_loss/len(train_loader):.4f}")

# ----- Save Model -----
torch.save(model.state_dict(), "safe_landing_model.pt")
print("✅ Model saved as safe_landing_model.pt")
