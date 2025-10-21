# pointnet_small.py
import torch
import torch.nn as nn
import torch.nn.functional as F

class PointNetSmall(nn.Module):
    def __init__(self, num_classes=2):
        super(PointNetSmall, self).__init__()
        self.conv1 = nn.Conv1d(3, 64, 1)
        self.conv2 = nn.Conv1d(64, 128, 1)
        self.conv3 = nn.Conv1d(128, 256, 1)
        self.fc1 = nn.Linear(256, 128)
        self.fc2 = nn.Linear(128, num_classes)
        self.dropout = nn.Dropout(0.3)
        self.bn1 = nn.BatchNorm1d(64)
        self.bn2 = nn.BatchNorm1d(128)
        self.bn3 = nn.BatchNorm1d(256)
        self.bn4 = nn.BatchNorm1d(128)

    def forward(self, x):
        x = x.transpose(2, 1)           # [B, 3, N]
        x = F.relu(self.bn1(self.conv1(x)))
        x = F.relu(self.bn2(self.conv2(x)))
        x = self.bn3(self.conv3(x))
        x = torch.max(x, 2)[0]          # global feature
        x = F.relu(self.bn4(self.fc1(x)))
        x = self.dropout(x)
        return self.fc2(x)