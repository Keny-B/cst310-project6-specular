# CST-310 Project 6 — Specular Lighting, Objects, Illumination & Shaders

**Student:** Keny Beauzile  
**Instructor:** Citro  
**Course:** Computer Graphics (CST-310)  
**Due:** October 26

Recreates the “shininess grid” (2, 4, 8, 16, 32, 64, 128, 256) using **C++/OpenGL 3.3** with a Phong lighting model.  
Eight cubes are rendered in a 2×4 layout; only the **specular exponent (shininess)** changes per cube.  
Numeric labels are drawn under each cube via a tiny GPU overlay (no font assets).

---

## Quick Start (Ubuntu / WSLg)

```bash
sudo apt update && sudo apt install -y build-essential libglfw3-dev libglew-dev libglm-dev
g++ -std=c++17 src/main.cpp -lglfw -lGLEW -lGL -ldl -lX11 -lpthread -lXrandr -lXi -o specular-grid
./specular-grid
