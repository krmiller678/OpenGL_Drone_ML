delta = 5

def left_right(current):
    global delta  
   
    x = current["x"]
    y = current["y"]
    z = current["z"]
    if x >= 955 or x <= 0:            
        delta = -delta
    return {"x": x + delta, "y": y, "z": z}
