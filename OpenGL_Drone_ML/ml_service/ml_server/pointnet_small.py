import torch
import torch.nn as nn
import torch.nn.functional as F

class PointNetSeg(nn.Module):
    def __init__(self, num_classes=2):
        super().__init__()
        self.conv1 = nn.Conv1d(5, 64, 1)
        self.conv2 = nn.Conv1d(64, 128, 1)
        self.conv3 = nn.Conv1d(128, 256, 1)
        self.bn1 = nn.BatchNorm1d(64)
        self.bn2 = nn.BatchNorm1d(128)
        self.bn3 = nn.BatchNorm1d(256)

        # Use LayerNorm for per-point FC features
        self.fc1 = nn.Linear(256, 128)
        self.ln1 = nn.LayerNorm(128)
        self.dropout = nn.Dropout(0.3)
        self.fc2 = nn.Linear(128, num_classes)

    def forward(self, x):
        # x: [B, N, 3]
        x = x.transpose(2,1)              # -> [B, 3, N]
        x = F.relu(self.bn1(self.conv1(x)))
        x = F.relu(self.bn2(self.conv2(x)))
        x = F.relu(self.bn3(self.conv3(x)))  # [B, 256, N]

        x = x.transpose(2,1)              # -> [B, N, 256]
        x = F.relu(self.ln1(self.fc1(x))) # [B, N, 128]
        x = self.dropout(x)
        x = self.fc2(x)                   # [B, N, num_classes]
        return x
