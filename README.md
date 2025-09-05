![made-with-c++](https://img.shields.io/badge/Made_with-C%2B%2B-blue)
![made-with-python](https://img.shields.io/badge/Made_with-Python-yellow)

<!-- LOGO -->
<br />
<h1>
<p align="center">
  <img src="" alt="Logo", width="50%", height ="auto">
  <br>
</h1>
  <p align="center">
    Real-time drone landing simulation with OpenGL + ML-based safe-zone detection
    <br />
    </p>
</p>
<p align="center">
  • <a href="#about-the-project">About The Project</a> •
  <a href="#quick-start">Quick Start</a> •
  <a href="#manually-build">Build</a> •
  <a href="#usage">How to Use</a> •
  <a href="#planned-features">Planned Features</a> •
  <a href="#acknowledgements">Acknowledgements</a> •
</p>  

<p align="center">
 
<img src="">
</p>                                                                                                                             
                                                                                                                                                      
## About The Project 
Our idea currently is to construct a drone simulation (probably in C++) that will simulate LiDAR scanning of a generated topography. From this, we hope to export a point cloud to an ML model that would advise the drone system on where best to land in the event of an emergency.

<a id="quick-start"></a>
## Quick Start 🚀

<a id="manually-build"></a>
## Manually Build 🛠️

Requirements:
- Cmake 3.16 or later
- C++17 or later

Clone from GitHub:
```
git clone https://github.com/krmiller678/OpenGL_Drone_ML <my-directory>
```

Install Dependencies:  

Run CMake from cloned directory:  
```bash
mkdir build && cd build
cmake ..
make
```

A note on deploying your own build:

## Usage
### Input/Output
- **Single-Click** 
- **Double-Click**
### On Screen Buttons
- **EXAMPLE** 

### Screen Elements
- **EXAMPLE**

**NOTE:** 

## Planned Features
- [ ] Generated Topography feature
- [ ] Manually engage emergency landing protocol 
- [ ] Select your Machine Learning Landing Algorithm
- [ ] Fuel level with automatic emergency landing

## Acknowledgements
- https://www.mdpi.com/2072-4292/13/10/1930
