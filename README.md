<div align="center">

# 🎮 Game AI — Agents & Navigation

**Algorithms 2 · Digital Arts and Entertainment (DAE), Howest**

[![Language](https://img.shields.io/badge/language-C%2B%2B-blue?style=flat-square&logo=cplusplus)](https://isocpp.org/)
[![Engine](https://img.shields.io/badge/engine-Unreal%20Engine-black?style=flat-square&logo=unrealengine)](https://www.unrealengine.com/)
[![License](https://img.shields.io/badge/license-Academic-green?style=flat-square)]()

*Autonomous agent behaviors and navigation systems for game development*

---

**Casper Van Laer**

</div>

## 📖 Overview

This project demonstrates the implementation of various **autonomous agent behaviors** and **navigation systems** commonly used in game development. Developed as part of the Algorithms 2 curriculum, the focus is on efficient movement, group dynamics, and complex pathfinding using both graph-based and mesh-based approaches.

---

## 🧠 Implemented AI Behaviors

### 1 · Steering Behaviors

Individual agent movement logic based on **Craig Reynolds' steering principles**.

| Behavior | Description |
|---|---|
| **Seek & Flee** | Moving directly toward or away from a target |
| **Arrive** | Gradual deceleration as the agent approaches a target |
| **Pursuit & Evade** | Predicting a target's future position to intercept or escape |
| **Wander** | Random but smooth movement using a projected circle and offset |

### 2 · Combined Steering

Complex behaviors created by blending multiple steering outputs.

- **Weighted Blending** — Combining behaviors like Seek and Avoidance with specific weights
- **Priority Steering** — Executing behaviors based on a hierarchy *(e.g., "Avoid Wall" takes priority over "Wander")*

### 3 · Flocking (Boids)

Simulating **emergent group behavior** through three fundamental rules:

> **Separation** · Avoid crowding local flockmates
>
> **Alignment** · Steer towards the average heading of local flockmates
>
> **Cohesion** · Steer toward the average position (center of mass) of local flockmates

### 4 · Spatial Partitioning

To optimize Flocking from **O(N²)** to **O(N · log N)** or better, I implemented **Cell Space Partitioning**. By dividing the environment into a grid, agents only check for neighbors within their own or adjacent cells — significantly boosting performance for large crowds.

---

## 🗺️ Navigation & Pathfinding

### 5 · Graph Theory & A* Pathfinding

The foundation of environment navigation using mathematical graphs.

- **Graph Construction** — Implementing nodes and weighted edges to represent traversable space
- **A\* Pathfinding** — An efficient search algorithm using heuristics *h(n)* and cost *g(n)* to find the shortest path between two points

### 6 · Navmesh Pathfinding

Advanced navigation using **Navigation Meshes** instead of simple grids.

- **Path Planning** — Finding a sequence of polygons from start to goal
- **Path Smoothing** — Using the **String Pulling (Funnel) algorithm** to convert a sequence of polygons into a natural, straight-line path for the agent

---

## 🏗️ Project Structure

```
Source/GameAIProg/
├── GraphTheory/            # Graph construction & algorithms (BFS, A*)
│   └── Algorithms/
├── Movement/
│   ├── Pathfinding/
│   │   ├── AStar/          # A* pathfinding implementation
│   │   └── Navmesh/        # Navigation mesh & funnel algorithm
│   └── SteeringBehaviors/
│       ├── Steering/       # Core steering behaviors
│       ├── CombinedSteering/
│       ├── Flocking/       # Boids simulation
│       ├── SpacePartitioning/
│       └── PathFollow/
└── Shared/                 # Common utilities
```

---

## 🛠️ Technical Details

| | |
|---|---|
| **Language** | C++ |
| **Framework** | Unreal Engine |
| **Optimization** | Cell Space Partitioning for neighbor detection |
| **Navigation** | Graph-based A\* → Navigation Meshes |

---

## 🚀 Getting Started

```bash
# Clone the repository
git clone https://github.com/DAE-GD-2025-2026/game-ai-project-CasperVanLaerHowest
```

1. Open `GameAIProg.sln` in **Visual Studio**
2. Ensure dependencies (SDL, Elite Framework) are correctly linked
3. Build and Run in **Release** mode for optimal performance during flocking simulations

---

## 📜 Credits

Developed by **Casper Van Laer** for the Digital Arts and Entertainment (DAE) program.

Special thanks to the DAE teaching team for the *"Elite"* project framework.
