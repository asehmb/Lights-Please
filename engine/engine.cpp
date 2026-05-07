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
  camera->position = mathplease::Vector3(0.0f, 0.0f, -2.0f);
  camera->pitch = 0.0f;
  camera->yaw = -M_PI;
  syncCurrentCameraState();
  previousCameraState = currentCameraState;

  hasCameraState = true;
  LOG_INFO("CAMERA",
           "Position: ({:.2f}, {:.2f}, {:.2f}), Pitch: {:.2f}, Yaw: {:.2f}",
           camera->position.x, camera->position.y, camera->position.z,
           camera->pitch * (180.0f / static_cast<float>(M_PI)),
           camera->yaw * (180.0f / static_cast<float>(M_PI)));

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

    inputState.beginFrame();

    platform::poll_events();

    inputState.updateFromPlatform();

    process_input();

    update(dt);
    float alpha = accumulator / fixedDt;
    render(1.0f);

    if (platform::should_close())
      is_running = false;
  }
  LOG_INFO("ENGINE", "Closing!");
}

void Engine::process_input() {

  pendingMouseDelta = pendingMouseDelta + inputState.mouseDelta;

  if (inputState.pressed(Key::P)) {
    toggleSimulationPaused();
  }

  if (inputState.pressed(Key::O)) {
    requestSingleStep();
  }

  if (inputState.pressed(Key::Num1)) {
    setSimulationTimescale(0.1f);
  } else if (inputState.pressed(Key::Num2)) {
    setSimulationTimescale(1.0f);
  } else if (inputState.pressed(Key::Num3)) {
    setSimulationTimescale(2.0f);
  }

  RenderSystem::PhysicsDebugVisualizationSettings debugSettings =
      renderSystem.getPhysicsDebugVisualizationSettings();

  bool debugSettingsChanged = false;

  if (inputState.pressed(Key::C)) {
    debugSettings.showColliderWireframes =
        !debugSettings.showColliderWireframes;
    debugSettingsChanged = true;
  }

  if (inputState.pressed(Key::B)) {
    debugSettings.showAABBs = !debugSettings.showAABBs;
    debugSettingsChanged = true;
  }

  if (inputState.pressed(Key::N)) {
    debugSettings.showContactPoints = !debugSettings.showContactPoints;
    debugSettingsChanged = true;
  }
  if (inputState.pressed(Key::F1)) {
    // dump camera info to log
    LOG_INFO("CAMERA",
             "Position: ({:.2f}, {:.2f}, {:.2f}), Pitch: {:.2f}, Yaw: {:.2f}",
             camera->position.x, camera->position.y, camera->position.z,
             camera->pitch * (180.0f / static_cast<float>(M_PI)),
             camera->yaw * (180.0f / static_cast<float>(M_PI)));
  }

  if (debugSettingsChanged) {
    renderSystem.setPhysicsDebugVisualizationSettings(debugSettings);

    LOG_INFO("PHYSICS_DEBUG", "Toggles collider={} aabb={} contacts={}",
             debugSettings.showColliderWireframes, debugSettings.showAABBs,
             debugSettings.showContactPoints);
  }
}

void Engine::update(float fixed_dt) {
  if (camera && hasCameraState) {
    previousCameraState = currentCameraState;
  }

  physicsSystem.update(*entity_manager_ptr, *sceneGraph, job_system.get(),
                       fixed_dt);
  gravitySystem.update(*entity_manager_ptr, job_system.get(), fixed_dt);
  sceneGraph->updateWorldTransforms(job_system.get());

  if (camera) {
    const mathplease::Vector3 forward = camera->getForward();
    const mathplease::Vector3 right = camera->getRight();
    const mathplease::Vector3 worldUp(0.0f, 1.0f, 0.0f);
    mathplease::Vector3 movement(0.0f, 0.0f, 0.0f);

    if (inputState.down(Key::W)) {
      movement = movement + forward;
    }
    if (inputState.down(Key::S)) {
      movement = movement - forward;
    }
    if (inputState.down(Key::D)) {
      movement = movement + right;
    }
    if (inputState.down(Key::A)) {
      movement = movement - right;
    }
    if (inputState.down(Key::Space)) {
      movement = movement + worldUp;
    }
    if (inputState.down(Key::Shift)) {
      movement = movement - worldUp;
    }

    if (movement.length() > 0.0f) {
      movement.normalize();
      camera->position =
          camera->position + movement * (camera->movementSpeed * fixed_dt);
    }
    const float mouseSensitivity = 0.0025f;
    const float pitchLimit = 1.55f; // slightly less than pi/2

    if (pendingMouseDelta.length() > 0.0f) {
      camera->yaw -= pendingMouseDelta.x * mouseSensitivity;
      camera->pitch -= pendingMouseDelta.y * mouseSensitivity;

      camera->pitch = std::clamp(camera->pitch, -pitchLimit, pitchLimit);

      pendingMouseDelta = mathplease::Vector2(0.0f, 0.0f);
    }

    syncCurrentCameraState();
    hasCameraState = true;
  }
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
