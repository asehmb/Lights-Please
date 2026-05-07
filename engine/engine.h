#pragma once

#include "asset/runtime_asset_registry.h"
#include "camera.h"
#include "entity/entity.h"
#include "entity/renderSystem.h"
#include "entity/sceneGraph.h"
#include "entity/systems.h"
#include "job_system.h"
#include "platform.h"
#include "renderer/renderer.h"
#include <functional>
#include <memory>

struct InputState {
  bool currentKeys[static_cast<int>(Key::COUNT)] = {};
  bool previousKeys[static_cast<int>(Key::COUNT)] = {};

  mathplease::Vector2 mouseDelta = mathplease::Vector2(0.0f, 0.0f);

  void beginFrame() {
    mouseDelta = mathplease::Vector2(0.0f, 0.0f);

    for (int i = 0; i < static_cast<int>(Key::COUNT); ++i) {
      previousKeys[i] = currentKeys[i];
    }
  }

  void updateFromPlatform() {
    for (int i = 0; i < static_cast<int>(Key::COUNT); ++i) {
      currentKeys[i] = platform::key_down(static_cast<Key>(i));
    }

    mouseDelta = mouseDelta + platform::get_relative_mouse_position();
  }

  bool down(Key key) const { return currentKeys[static_cast<int>(key)]; }

  bool pressed(Key key) const {
    int idx = static_cast<int>(key);
    return currentKeys[idx] && !previousKeys[idx];
  }

  bool released(Key key) const {
    int idx = static_cast<int>(key);
    return !currentKeys[idx] && previousKeys[idx];
  }
};

class Engine {
public:
  using FrameUpdateHook = std::function<void(Engine &)>;

  void run();
  void initialize();
  ~Engine();

  // Getters for application access
  Renderer *getRenderer() const { return renderer.get(); }
  EntityManager *getEntityManager() { return entity_manager_ptr.get(); }
  SceneGraph *getSceneGraph() { return sceneGraph.get(); }
  RenderSystem *getRenderSystem() { return &renderSystem; }
  RuntimeAssetRegistry *getAssetRegistry() { return &assetRegistry; }
  Camera *getCamera() const { return camera.get(); }
  JobSystem *getJobSystem() const { return job_system.get(); }
  void setSimulationPaused(bool paused);
  void toggleSimulationPaused();
  void requestSingleStep();
  void setSimulationTimescale(float timescale);
  bool isSimulationPaused() const { return simulationPaused; }
  float getSimulationTimescale() const { return simulationTimescale; }
  void setFrameUpdateHook(FrameUpdateHook hook) {
    frameUpdateHook = std::move(hook);
  }
  InputState inputState;

private:
  struct CameraState {
    mathplease::Vector3 position{};
    float pitch = 0.0f;
    float yaw = 0.0f;
  };

  void process_input();
  void update(float fixed_dt); // Fixed logic (Physics, AI)
  void render(float alpha);    // Variable rendering (Graphics)
  void syncCurrentCameraState();
  mathplease::Vector2 pendingMouseDelta{};
  std::unique_ptr<EntityManager> entity_manager_ptr;
  std::unique_ptr<SceneGraph> sceneGraph;
  PhysicsSystem physicsSystem;
  GravitySystem gravitySystem;
  RenderSystem renderSystem;

  bool is_running = false;
  const float dt = 1.0f / 60.0f; // Target 60Hz for logic
  bool simulationPaused = false;
  bool singleStepRequested = false;
  float simulationTimescale = 1.0f;
  bool previousKeyStates[static_cast<int>(Key::COUNT)]{};
  std::shared_ptr<Camera> camera;
  CameraState previousCameraState;
  CameraState currentCameraState;
  bool hasCameraState = false;
  std::unique_ptr<JobSystem> job_system;
  std::unique_ptr<Renderer> renderer;
  RuntimeAssetRegistry assetRegistry;
  FrameUpdateHook frameUpdateHook;
};
