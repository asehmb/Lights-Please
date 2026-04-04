#include "engine.h"
#include "logger.h"
#include "platform.h"
#include <algorithm>
#include <cstring>
#include <memory>

void Engine::initialize() {
  is_running = true;
  platform::init(1280, 720);
  std::memset(previousKeyStates, 0, sizeof(previousKeyStates));

  // create job system
  job_system = std::make_unique<JobSystem>();
  job_system->initialize(0);

  // create camera
  camera = std::make_shared<Camera>();
  syncCurrentCameraState();
  previousCameraState = currentCameraState;
  hasCameraState = true;

  // create entity manager
  entity_manager_ptr = std::make_unique<EntityManager>();
  sceneGraph = std::make_unique<SceneGraph>(*entity_manager_ptr);
  entity_manager_ptr->setEntityDestroyedCallback(
      [this](Entity_id, Transform_id transformHandle) {
        if (!sceneGraph || transformHandle == NULL_ENTITY) {
          return;
        }
        sceneGraph->deleteTransform(transformHandle);
      });

  // create renderer from platform window
  renderer = std::make_unique<Renderer>(platform::get_window_ptr());
  renderer->setCamera(camera);

  LOG_INFO("ENGINE", "Engine initialized");
}

Engine::~Engine() {
  // Cleanup if necessary
  is_running = false;
  if (entity_manager_ptr) {
    entity_manager_ptr->setEntityDestroyedCallback({});
  }
}
// Update order:
// input
// gameplay intent
// physics step(s)
// transform sync
// render submission
void Engine::run() {
  is_running = true;

  float accumulator = 0.0f;
  const float fixedDt = dt; // Fixed timestep of 60 FPS

  // Reset platform timer to avoid large delta on first frame
  platform::delta_time();

  while (is_running) {
    float frame_time = platform::delta_time();

    // Prevent "Spiral of Death" (if game lags, don't try to catch up forever)
    if (frame_time > 0.25f)
      frame_time = 0.25f;

    accumulator += frame_time * simulationTimescale;

    process_input();

    if (singleStepRequested) {
      update(fixedDt);
      singleStepRequested = false;
      simulationPaused = true;
      accumulator = 0.0f;
    }

    // If we have enough unsimulated time, run fixed steps
    while (!simulationPaused && accumulator >= fixedDt) {
      update(fixedDt);
      accumulator -= fixedDt;
    }

    // 'alpha' is how far we are between the current and next physics state
    // This is used for "interpolation" to make motion look smooth
    float alpha = accumulator / fixedDt;
    render(alpha);

    if (platform::should_close())
      is_running = false;
  }
  LOG_INFO("ENGINE", "Closing!");
}

void Engine::process_input() {
  platform::poll_events();
  pendingMouseDelta =
      pendingMouseDelta + platform::get_relative_mouse_position();

  auto keyPressedThisFrame = [this](Key key) {
    const int idx = static_cast<int>(key);
    const bool down = platform::key_down(key);
    const bool pressed = down && !previousKeyStates[idx];
    previousKeyStates[idx] = down;
    return pressed;
  };

  if (keyPressedThisFrame(Key::P)) {
    toggleSimulationPaused();
  }

  if (keyPressedThisFrame(Key::O)) {
    requestSingleStep();
  }

  if (keyPressedThisFrame(Key::Num1)) {
    setSimulationTimescale(0.1f);
  } else if (keyPressedThisFrame(Key::Num2)) {
    setSimulationTimescale(1.0f);
  } else if (keyPressedThisFrame(Key::Num3)) {
    setSimulationTimescale(2.0f);
  }

  RenderSystem::PhysicsDebugVisualizationSettings debugSettings =
      renderSystem.getPhysicsDebugVisualizationSettings();
  bool debugSettingsChanged = false;
  if (keyPressedThisFrame(Key::C)) {
    debugSettings.showColliderWireframes =
        !debugSettings.showColliderWireframes;
    debugSettingsChanged = true;
  }
  if (keyPressedThisFrame(Key::B)) {
    debugSettings.showAABBs = !debugSettings.showAABBs;
    debugSettingsChanged = true;
  }
  if (keyPressedThisFrame(Key::N)) {
    debugSettings.showContactPoints = !debugSettings.showContactPoints;
    debugSettingsChanged = true;
  }
  if (debugSettingsChanged) {
    renderSystem.setPhysicsDebugVisualizationSettings(debugSettings);
    LOG_INFO("PHYSICS_DEBUG", "Toggles collider={} aabb={} contacts={}",
             debugSettings.showColliderWireframes, debugSettings.showAABBs,
             debugSettings.showContactPoints);
  }
}

void Engine::update(float fixed_dt) {
  // Update game logic, physics, AI, etc. here
  if (camera && hasCameraState) {
    previousCameraState = currentCameraState;
  }

  // Run systems
  physicsSystem.update(*entity_manager_ptr, *sceneGraph, job_system.get(),
                       fixed_dt);
  gravitySystem.update(*entity_manager_ptr, job_system.get(), fixed_dt);
  sceneGraph->updateWorldTransforms(job_system.get());

  // camera->update(fixed_dt);
  const mathplease::Vector3 moveDelta = camera->velocity * fixed_dt;
  if (platform::key_down(Key::Escape)) {
    is_running = false;
  }
  if (platform::key_down(Key::W)) {
    camera->position.z -= moveDelta.z;
  }
  if (platform::key_down(Key::S)) {
    camera->position.z += moveDelta.z;
  }
  if (platform::key_down(Key::A)) {
    camera->position.x -= moveDelta.x;
  }
  if (platform::key_down(Key::D)) {
    camera->position.x += moveDelta.x;
  }
  if (platform::key_down(Key::Space)) {
    camera->position.y += moveDelta.y;
  }
  if (platform::key_down(Key::Shift)) {
    camera->position.y -= moveDelta.y;
  }
  const mathplease::Vector2 mouseDelta = pendingMouseDelta;
  pendingMouseDelta = mathplease::Vector2(0.0f, 0.0f);
  camera->yaw -= mouseDelta.x * camera->mouseSensitivity;
  const float pitchSign = camera->invertY ? 1.0f : -1.0f;
  camera->pitch += mouseDelta.y * camera->mouseSensitivity * pitchSign;
  const float pitchLimitRad =
      camera->pitchConstraint * static_cast<float>(M_PI / 180.0);
  camera->pitch = std::clamp(camera->pitch, -pitchLimitRad, pitchLimitRad);

  if (frameUpdateHook) {
    frameUpdateHook(*this);
  }

  syncCurrentCameraState();
  hasCameraState = true;
}

void Engine::render(float alpha) {
  // Render the current frame here
  const float interpolationAlpha = std::clamp(alpha, 0.0f, 1.0f);
  if (renderer) {
    if (camera && hasCameraState) {
      const mathplease::Vector3 interpolatedCameraPosition =
          previousCameraState.position +
          (currentCameraState.position - previousCameraState.position) *
              interpolationAlpha;
      const float interpolatedPitch =
          previousCameraState.pitch +
          (currentCameraState.pitch - previousCameraState.pitch) *
              interpolationAlpha;
      const float interpolatedYaw =
          previousCameraState.yaw +
          (currentCameraState.yaw - previousCameraState.yaw) *
              interpolationAlpha;

      const mathplease::Matrix4 rotation =
          mathplease::Matrix4::rotateY(interpolatedYaw) *
          mathplease::Matrix4::rotateX(interpolatedPitch);
      const mathplease::Matrix4 view =
          rotation.transposed() *
          mathplease::Matrix4::translate(-interpolatedCameraPosition);
      renderer->setFrameCameraMatrices(view, camera->getProjectionMatrix());
    }

    renderSystem.update(*entity_manager_ptr, *sceneGraph, *renderer,
                        interpolationAlpha);
    renderer->drawFrame();
  } else {
    LOG_ERR("ENGINE", "Renderer doesnt exist??");
  }
}

void Engine::syncCurrentCameraState() {
  if (!camera) {
    return;
  }
  currentCameraState.position = camera->position;
  currentCameraState.pitch = camera->pitch;
  currentCameraState.yaw = camera->yaw;
}

void Engine::setSimulationPaused(bool paused) {
  simulationPaused = paused;
  if (!simulationPaused) {
    singleStepRequested = false;
  }
  LOG_INFO("SIM", "Simulation {}", simulationPaused ? "paused" : "running");
}

void Engine::toggleSimulationPaused() {
  setSimulationPaused(!simulationPaused);
}

void Engine::requestSingleStep() {
  singleStepRequested = true;
  simulationPaused = true;
  LOG_INFO("SIM", "Queued single-step physics tick");
}

void Engine::setSimulationTimescale(float timescale) {
  simulationTimescale = std::clamp(timescale, 0.0f, 2.0f);
  LOG_INFO("SIM", "Timescale set to {:.1f}x", simulationTimescale);
}
