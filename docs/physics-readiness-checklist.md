# Physics-Ready Milestone Checklist

This checklist is scoped to the current `Lights Please` codebase and ordered so you can move into physics implementation with minimal rework.

## Goal

By the end of this milestone, the engine should support deterministic fixed-step simulation, physics-ready ECS data, stable transform propagation, and basic debug visibility.

## Day 1: Core Runtime Readiness

### 1. Fix the simulation loop to true fixed timestep

- File: `engine/engine.cpp`
- Change:
  - Run simulation with constant `dt` (`1.0f / 60.0f`) inside the accumulator loop.
  - Use `while (accumulator >= dt)` instead of `while (accumulator >= frame_time)`.
  - Compute interpolation alpha as `accumulator / dt`.
- Why:
  - Physics must step deterministically and not depend on frame-time jitter.
- Acceptance criteria:
  - Same inputs produce stable movement across variable framerates.
  - No runaway stepping during frame spikes.

### 2. Add explicit `Transform` component support

- Files: `engine/entity/entity.h`, `engine/entity/entity.cpp`, systems area
- Change:
  - Introduce a transform component with:
    - translation (`Vector3`/`Vector4`)
    - rotation (quaternion preferred)
    - scale
    - cached local/world matrix
  - Keep existing `Position` for compatibility or migrate fully to `Transform`.
- Why:
  - Physics and rendering both need consistent spatial data.
- Acceptance criteria:
  - Renderables read world transform from one authoritative component path.

### 3. Define physics-facing ECS components (data only)

- Files: `engine/entity/entity.h` (+ related ECS registration code)
- Add components:
  - `RigidBody`: mass, inverseMass, velocity, angularVelocity, damping, bodyType
  - `Collider`: shape type + parameters (AABB or sphere first), layer, mask, isTrigger
  - `PhysicsMaterial`: restitution, friction
- Why:
  - Lets you wire gameplay and authoring before solving full physics behavior.
- Acceptance criteria:
  - Entities can be created with these components.
  - Component data can be queried without crashes.

### 4. Lock global simulation conventions

- Create doc: `docs/physics-conventions.md`
- Define:
  - World unit (`1.0 = 1 meter`)
  - Gravity default (`-9.81` on chosen up-axis)
  - Axis convention (Y-up or Z-up)
  - Handedness and rotation convention
- Why:
  - Prevents costly rework when importing assets and tuning gameplay.
- Acceptance criteria:
  - Team can implement physics and gameplay against one written convention.

## Day 2: Integration and Debug Readiness

### 5. Add collision filtering infrastructure

- Files: ECS component defs + future physics system API
- Change:
  - Implement layer/mask bit filtering rules in component data.
  - Reserve behavior flags for `trigger` vs `solid`.
- Why:
  - Needed immediately once broadphase/narrowphase starts.
- Acceptance criteria:
  - Simple function can answer: “Should A collide with B?”

### 6. Add a minimal transform/physics update order contract

- Files: `engine/engine.cpp` or system scheduler area
- Change:
  - Establish and document update order:
    - input
    - gameplay intent
    - physics step(s)
    - transform sync
    - render submission
- Why:
  - Prevents frame-lag bugs and double-writes to transforms.
- Acceptance criteria:
  - One frame has one clear source of truth for transforms before render.

### 7. Add physics debug visualization hooks

- Files: renderer debug path + render system integration
- Change:
  - Add toggles for:
    - collider wireframes
    - AABBs
    - contact points (placeholder if contacts not implemented yet)
- Why:
  - Physics work is too slow to iterate without visualization.
- Acceptance criteria:
  - Debug draw can be toggled at runtime.

### 8. Add simulation control tools

- Files: input/engine loop area
- Add:
  - pause simulation
  - single-step one physics tick
  - optional timescale (0.1x, 1x, 2x)
- Why:
  - Essential for validating solver behavior and collisions.
- Acceptance criteria:
  - Can freeze and step simulation deterministically.

## Nice-to-Have (Do if time remains)

- Add an `AssetRegistry` entry path for collider metadata per mesh.
- Add deterministic test harness for stepping N fixed ticks and validating expected positions.
- Add profiler markers for fixed update, broadphase, narrowphase, and solver timings.

## Exit Criteria Before Starting Full Physics

- Fixed-step loop is correct and validated.
- Transform flow is authoritative and stable.
- Physics component schema exists in ECS.
- Collision filtering data path is present.
- Debug pause/step and basic physics visualization are available.
- Conventions are written in docs.

