# train_pointnet.py
import torch
import torch.nn as nn
import torch.optim as optim
from torch.utils.data import Dataset, DataLoader
import numpy as np
from pointnet_small import PointNetSmall

# -------- Mock Data Generator --------
def generate_point_cloud(flat=True, noise=0.01):
    x, z = np.meshgrid(np.linspace(-1, 1, 5), np.linspace(-1, 1, 5))
    if flat:
        y = np.random.normal(0, noise, x.shape)
    else:
        y = 0.5 * x + 0.3 * z + np.random.normal(0, noise, x.shape)
    points = np.stack([x, y, z], axis=-1).reshape(-1, 3)
    label = 1 if flat else 0
    return points, label

class LidarDataset(Dataset):
    def __init__(self, n_samples=2000):
        self.data = [generate_point_cloud(flat=np.random.rand() > 0.5) for _ in range(n_samples)]
    def __len__(self): return len(self.data)
    def __getitem__(self, idx):
        points, label = self.data[idx]
        return torch.tensor(points, dtype=torch.float32), torch.tensor(label, dtype=torch.long)

# -------- Training --------
dataset = LidarDataset()
train_loader = DataLoader(dataset, batch_size=32, shuffle=True)

model = PointNetSmall()
criterion = nn.CrossEntropyLoss()
optimizer = optim.Adam(model.parameters(), lr=0.001)

for epoch in range(30):
    total_loss = 0
    for pts, labels in train_loader:
        optimizer.zero_grad()
        out = model(pts)
        loss = criterion(out, labels)
        loss.backward()
        optimizer.step()
        total_loss += loss.item()
    print(f"Epoch {epoch+1}/30 | Loss: {total_loss/len(train_loader):.4f}")

torch.save(model.state_dict(), "safe_landing_model.pt")
print("✅ Model saved as safe_landing_model.pt")