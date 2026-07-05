#  Real-Time 2D Fluid Simulation (SPH)

A real-time, interactive **2D fluid simulator** written in modern **C++17** and **OpenGL**, based on **Smoothed Particle Hydrodynamics (SPH)**. Thousands of particles are simulated on the CPU and rendered on the GPU using **instanced rendering**, with a live control panel for tuning every physical parameter on the fly.

The simulation reproduces realistic fluid behavior — pressure, incompressibility, viscosity, gravity and wall collisions — and lets you push, pull and stir the liquid directly with the mouse.

---

##  Overview

Each particle is a small circle colored by its speed (a blue → cyan → green → yellow → red heat map). As the fluid flows, splashes and settles, the color field makes the velocity distribution of the flow immediately readable.

> _Tip: add a GIF or screenshot here to showcase the simulation, e.g._ `![demo](docs/demo.gif)`

---

##  Features

- **Physically-based SPH solver** — density, pressure and near-pressure forces produce a convincing incompressible liquid.
- **Dual-pressure model** — a standard pressure term keeps target density, while a separate **near-density** term prevents particle clumping and interpenetration for crisp, stable fluids.
- **Viscosity** — velocity-smoothing between neighbors gives the fluid its "thickness" (from water-like to honey-like).
- **Spatial hashing acceleration** — a hash-grid neighbor search reduces the naïve O(n²) interaction cost to near O(n), enabling thousands of particles in real time.
- **Predicted positions** — forces are evaluated on look-ahead positions for a more stable integration step.
- **Interactive mouse forces** — **left-click to attract**, **right-click to repel** the fluid within an adjustable radius.
- **GPU instanced rendering** — all particles are drawn in a single `glDrawArraysInstanced` call; a fragment shader renders each as an anti-aliased circle.
- **Velocity heat-map coloring** — particle color encodes speed, visualizing the flow field in real time.
- **Live ImGui control panel** — tweak gravity, smoothing radius, density, pressure, viscosity, damping, interaction strength, particle count and radius while the simulation runs.
- **Start / Pause / Reset** controls and on-the-fly particle-count changes (10 – 2000 particles).
- **Boundary collisions** with configurable damping (energy loss on wall bounce).
- **Multi-threaded physics** — the hot loops are parallelized with OpenMP `#pragma omp parallel for`.

---

##  How It Works

The core of the simulation is the **SPH** method, where a continuous fluid is approximated by discrete particles. Each simulation step performs:

1. **Apply gravity & predict positions** — external forces are applied and each particle's near-future position is estimated.
2. **Rebuild spatial lookup** — particles are hashed into grid cells and sorted so neighbors can be found quickly.
3. **Compute density** — for every particle, density is summed from neighbors within the smoothing radius using a smoothing kernel.
4. **Compute forces** — pressure (from density error), near-pressure (anti-clumping), and viscosity forces are accumulated from neighbors; mouse interaction forces are added when clicking.
5. **Integrate & resolve collisions** — velocities and positions are updated, and particles are clamped inside the domain with damping.
6. **Render** — updated positions and speeds are streamed to GPU buffers and drawn as instanced, color-coded circles.

### Smoothing kernels
The solver uses several tuned kernels and their analytical derivatives:
- **Poly-style density kernel** — smooth density estimation.
- **Near-density kernel** (`(r − d)³`) — strong short-range repulsion.
- **Viscosity kernel** — neighbor velocity smoothing.

---

##  Tech Stack

| Component            | Technology                                   |
| ------------------- | -------------------------------------------- |
| Language            | C++17                                         |
| Graphics API        | OpenGL 3.3 (Core Profile)                     |
| Windowing / input   | GLFW                                          |
| GL loader           | GLAD                                          |
| GUI                 | Dear ImGui                                    |
| Parallelism         | OpenMP                                         |
| Build system        | CMake (≥ 3.27)                                 |
| Rendering technique | Instanced rendering + custom GLSL shaders     |

---

##  Project Structure

```
Fluid_Simulation/
├── main.cpp          # Application entry point, main loop, ImGui UI, mouse interaction
├── Physics.cpp/.h    # SPH solver: density, pressure, viscosity, spatial hashing, collisions
├── Render.cpp/.h     # Shader compilation, VAO/VBO setup, instanced rendering data
├── Common.h          # Shared structs (Vec2, Particle, spatial-hash Entry) and constants
├── shader.vs         # Vertex shader — positions each instanced quad
├── shader.fs         # Fragment shader — draws anti-aliased circle + velocity heat map
├── glad.c            # GLAD OpenGL loader
├── includes/         # Third-party headers (GLFW, GLAD, ImGui, stb_image)
└── CMakeLists.txt    # Build configuration
```

---

##  Building & Running

### Prerequisites
- A C++17 compiler (Clang / GCC / MSVC)
- [CMake](https://cmake.org/) ≥ 3.27
- [GLFW](https://www.glfw.org/) 3.3+ installed and discoverable by CMake
- OpenGL drivers (available on virtually all modern systems)

### Build

```bash
git clone https://github.com/<your-username>/Fluid_Simulation.git
cd Fluid_Simulation

mkdir build && cd build
cmake ..
cmake --build .
```

### Run

The executable loads its shaders from the parent directory (`../shader.vs`, `../shader.fs`), so run it from the `build/` folder:

```bash
./FluidSimulation
```

Press **Start** in the control panel to begin the simulation, then click and drag inside the window to interact with the fluid.

---

##  Controls

| Input                       | Action                                    |
| --------------------------- | ----------------------------------------- |
| **Left mouse button**       | Attract particles toward the cursor       |
| **Right mouse button**      | Repel particles away from the cursor      |
| **Start / Pause** button    | Run or freeze the simulation              |
| **Reset** button            | Restart particles in their initial grid   |
| **Sliders**                 | Adjust physics parameters in real time    |

### Tunable parameters
Gravity · Smoothing Radius · Target Density · Pressure Multiplier · Viscosity · Collision Damping · Interaction Radius · Interaction Strength · Particle Count · Particle Radius.

---

##  Highlights / What This Project Demonstrates

- Implementation of a **non-trivial physics algorithm (SPH)** from the underlying math — kernels, density fields, pressure gradients and viscosity — rather than an off-the-shelf engine.
- **Performance engineering**: spatial hashing for neighbor search and OpenMP multi-threading to keep thousands of interacting particles running at interactive frame rates.
- **Modern OpenGL graphics programming**: core-profile pipeline, custom GLSL vertex/fragment shaders, instanced rendering, and GPU buffer streaming.
- **Clean separation of concerns**: physics, rendering and application logic split into focused modules.
- **Interactive tooling**: a real-time parameter-tuning UI that turns the project into an explorable sandbox.
- Practical integration of a **C++ toolchain** — CMake, GLFW, GLAD, Dear ImGui and OpenMP working together across platforms.

---

##  Possible Future Improvements

- Fully **GPU-based** solver (compute shaders) for even larger particle counts.
- Surface reconstruction / metaball rendering for a continuous liquid look.
- 3D extension of the solver.
- Obstacles and custom container shapes.
- Saving/loading of parameter presets.

---

##  References & Inspiration

- Smoothed Particle Hydrodynamics (SPH) fluid dynamics literature.
- Sebastian Lague's *Coding Adventure: Simulating Fluids* — a well-known reference for real-time particle-based fluids.

---

> Built as a hands-on exploration of real-time physics simulation and graphics programming in C++.
