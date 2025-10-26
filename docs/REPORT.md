


# docs/REPORT.md

```md
# Project 6 — Specular Lighting, Objects, Illumination & Shaders
**Student:** Keny Beauzile • **Instructor:** Citro • **Course:** CST-310  
**Due:** October 26

## 1) Objective
Reproduce the classic shininess grid (2..256) and analyze how the **Phong specular exponent** shapes the highlight. The only parameter that changes between cubes is the **material shininess**; geometry, view, and light stay identical.

---

## 2) Theoretical Background (concise but rigorous)

### 2.1 Phong Reflection Model (per fragment)
We compute color as the sum of ambient, diffuse (Lambert), and specular terms:
\[
I = k_a \, L_a + k_d \, L_d \,\max(\mathbf{n}\cdot \mathbf{l}, 0) + k_s \, L_s \,\max(\mathbf{v}\cdot \mathbf{r}, 0)^{\alpha}
\]
- \(\mathbf{n}\): unit surface normal; \(\mathbf{l}\): unit light direction; \(\mathbf{v}\): unit view direction.  
- \(\mathbf{r} = \text{reflect}(-\mathbf{l}, \mathbf{n}) = 2(\mathbf{n}\cdot\mathbf{l})\mathbf{n}-\mathbf{l}\).  
- \(k_a,k_d,k_s\): material ambient/diffuse/specular colors; \(L_a,L_d,L_s\): light colors.  
- \(\alpha\) (**shininess**) controls highlight **concentration**: larger \(\alpha\) → smaller, tighter, brighter hotspot.

### 2.2 Normal Transform
Because model transforms may shear/scale, normals use the **inverse-transpose** of the upper-left 3×3 of the model matrix:
\[
\mathbf{n}' = (\mathrm{M}^{-1})^{\top}_{3×3} \mathbf{n}
\]
(Implemented as `mat3(transpose(inverse(model)))`.)

### 2.3 Why the bottom row dots are smaller
For identical light/view/normal configurations, the angular falloff of \(\max(\mathbf{v}\cdot\mathbf{r},0)^{\alpha}\) collapses rapidly as \(\alpha\) grows, so **FWHM** of the specular lobe shrinks roughly like \(\alpha^{-1/2}\) for Phong-like lobes. Hence 32→256 shows visibly **sharper** points.

---

## 3) Implementation Details

**Language / API:** C++17, OpenGL 3.3 Core, GLFW + GLEW + GLM.  
**Shaders:** `phong.vert` / `phong.frag` (per-fragment Phong).  
**Geometry:** Unit cube (36 vertices, per-face normals).  
**Transforms:**  
- `projection = perspective(45°, 1280/720, 0.1, 100)`  
- `view = lookAt([0,0,8.8], [0,0,0], [0,1,0])`  
- `model = translate(cols[c], rows[r], 0) * baseRotation * scale(1.36)`

**Light placement (for left-of-center hotspot):** `light.position = (-2.2, 1.9, 3.6)`  
**Material:**  
- `ambient = (0.20, 0.10, 0.06)`  
- `diffuse = (1.00, 0.50, 0.31)` (warm orange)  
- `specular = (0.55, 0.55, 0.55)`  
**Background:** `clear = (0.18, 0.20, 0.22, 1)`

**Shininess values per cube:** `[2, 4, 8, 16, 32, 64, 128, 256]`  
**Layout:** 2 rows × 4 columns with fixed rotations to mimic the reference.  
**Labels:** A minimal seven-segment overlay in clip-space draws the numbers (no font textures).

---

## 4) Algorithms / Flow

**A. Rendering pipeline (per frame)**
1. Clear color/depth (dark gray).
2. For `i in 0..7`:
   - Compute `model` (row/col placement, base rotation, scale).
   - Set `material.shininess = shininess[i]`.
   - Draw cube (36 vertices, depth test on).
3. Disable depth; render 2D numeric labels in an orthographic pass.
4. Swap buffers.

**B. Per-fragment lighting (`phong.frag`)**
1. `N = normalize(Normal)`  
2. `L = normalize(light.pos - FragPos)`  
3. `V = normalize(viewPos - FragPos)`  
4. `diff = max(dot(N,L), 0)`  
5. `R = reflect(-L, N)`  
6. `spec = pow(max(dot(V,R),0), shininess)`  
7. Combine ambient + diffuse + specular → `FragColor`.

---

## 5) Aesthetic & Matching Decisions

- **Hotspot position**: light is slightly left/above/forward so the bright region appears a bit **inside** the front face and biased to the **left edge**, matching the sample.
- **Color**: warm diffuse orange plus neutral white light; ambient is low but non-zero to retain shadowed edges.
- **Labels**: white, subtle, centered; bottom row typeset a touch larger like the sample.

---

## 6) Validation

- Visual check: shrinking hotspot size as \(\alpha\) increases (clearly visible bottom row).  
- Consistency: all cubes share identical transforms and material except `shininess`.  
- If the highlight drifts or looks too broad on your machine, tweak only:
  - `light.position` (±0.3) or the yaw/pitch in `baseRotation`.

---

## 7) Build & Run

```bash
sudo apt install -y build-essential libglfw3-dev libglew-dev libglm-dev
g++ -std=c++17 src/main.cpp -lglfw -lGLEW -lGL -ldl -lX11 -lpthread -lXrandr -lXi -o specular-grid
./specular-grid
