import math

def distance(p1, p2):
    """Euclidean distance in 2D."""
    return math.sqrt((p1["x"] - p2["x"])**2 + (p1["y"] - p2["y"])**2)


