# Rendering Implementation (Current)

## High-level flow
1. `main.cpp` bootstraps `Engine` and creates GPU resources (`Material`, `Mesh`) through `Renderer` helpers.
2. `main.cpp` asks `RenderSystem::createRenderableEntity(...)` to create ECS render entities.
3. `RenderSystem` auto-registers mesh/material pointers and reuses existing IDs if the same pointers were already registered.
4. During each frame, `Engine::render()` calls `RenderSystem::update(entityManager, renderer)`.
5. `RenderSystem::update`:
   - queries ECS archetypes containing `Position | Renderable`
   - resolves `meshId/materialId` to pointers from its internal registries
   - builds `Renderer::Drawable` objects with transforms from `Position`
   - sorts drawables by material/mesh pointer for more coherent submission
   - clears renderer drawables and re-submits the current frame list
6. `Renderer::drawFrame()` records Vulkan commands over submitted drawables, binds each drawable's pipeline and descriptor sets, and issues mesh draw calls.

## Ownership model
- `EntityManager`: owns ECS data (archetypes/chunks/components) only.
- `RenderSystem`: owns render asset registries (`Mesh*` and `Material*` mappings + auto IDs).
- `Renderer`: owns Vulkan state/resources and consumes drawables each frame.

## Why this structure
- Keeps ECS generic and renderer-agnostic.
- Centralizes render registration/deduplication so `main.cpp` no longer manages IDs.
- Makes frame submission deterministic: renderables are reconstructed from ECS every frame.
