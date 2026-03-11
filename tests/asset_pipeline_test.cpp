#include "../engine/asset/asset_pipeline.h"
#include "../engine/job_system.h"
#include <cassert>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <thread>

namespace {

void writeTestObj(const std::filesystem::path &path) {
  std::ofstream out(path, std::ios::trunc);
  assert(out.good());
  out << "v 0.0 0.0 0.0\n";
  out << "v 1.0 0.0 0.0\n";
  out << "v 0.0 1.0 0.0\n";
  out << "vt 0.0 0.0\n";
  out << "vt 1.0 0.0\n";
  out << "vt 0.0 1.0\n";
  out << "vn 0.0 0.0 1.0\n";
  out << "f 1/1/1 2/2/1 3/3/1\n";
}

void writeTestPpm(const std::filesystem::path &path, std::uint8_t base) {
  std::ofstream out(path, std::ios::binary | std::ios::trunc);
  assert(out.good());
  out << "P6\n2 2\n255\n";
  const unsigned char pixels[] = {
      base, 0, 0, static_cast<unsigned char>(base + 10), 0, 0,
      static_cast<unsigned char>(base + 20), 0, 0,
      static_cast<unsigned char>(base + 30), 0, 0};
  out.write(reinterpret_cast<const char *>(pixels), sizeof(pixels));
}

bool waitForAssetLoads(asset::AssetPipeline &pipeline,
                       const std::vector<asset::AssetUUID> &uuids,
                       std::chrono::milliseconds timeout) {
  const auto start = std::chrono::steady_clock::now();
  while (std::chrono::steady_clock::now() - start < timeout) {
    pipeline.pollCompletedLoads();
    bool allReady = true;
    for (const auto &uuid : uuids) {
      if (!pipeline.tryGetAsset(uuid)) {
        allReady = false;
        break;
      }
    }
    if (allReady) {
      return true;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(2));
  }
  return false;
}

} // namespace

int main() {
  const std::filesystem::path tempRoot =
      std::filesystem::temp_directory_path() / "lights_please_asset_pipeline_test";
  std::filesystem::remove_all(tempRoot);
  std::filesystem::create_directories(tempRoot);

  const std::filesystem::path meshSource = tempRoot / "triangle.obj";
  const std::filesystem::path textureSource = tempRoot / "albedo.ppm";
  writeTestObj(meshSource);
  writeTestPpm(textureSource, 10);

  JobSystem jobSystem;
  jobSystem.initialize(2);
  asset::AssetPipeline pipeline(&jobSystem);

  const asset::AssetUUID textureUuid =
      pipeline.registerAsset(asset::AssetType::Texture, textureSource);
  const asset::AssetUUID meshUuid =
      pipeline.registerAsset(asset::AssetType::Mesh, meshSource, {}, {textureUuid});

  assert(textureUuid != meshUuid);
  assert(pipeline.hasAsset(textureUuid));
  assert(pipeline.hasAsset(meshUuid));
  assert(pipeline.findBySourcePath(textureSource).has_value());

  const auto beforeRequest = std::chrono::steady_clock::now();
  const bool textureScheduled = pipeline.requestLoad(textureUuid);
  const bool meshScheduled = pipeline.requestLoad(meshUuid);
  const auto requestDuration = std::chrono::steady_clock::now() - beforeRequest;
  assert(textureScheduled);
  assert(meshScheduled);
  assert(requestDuration < std::chrono::milliseconds(25));
  assert(pipeline.pendingLoadCount() >= 1);

  assert(waitForAssetLoads(pipeline, {textureUuid, meshUuid},
                           std::chrono::seconds(2)));

  const auto texturePayload = pipeline.tryGetAsset(textureUuid);
  const auto meshPayload = pipeline.tryGetAsset(meshUuid);
  assert(texturePayload);
  assert(meshPayload);
  assert(std::holds_alternative<asset::TextureAssetData>(*texturePayload));
  assert(std::holds_alternative<asset::MeshAssetData>(*meshPayload));

  const auto textureView = pipeline.getRecord(textureUuid);
  const auto meshView = pipeline.getRecord(meshUuid);
  assert(textureView.has_value());
  assert(meshView.has_value());
  assert(std::filesystem::exists(textureView->binaryPath));
  assert(std::filesystem::exists(meshView->binaryPath));

  const std::uint64_t initialTextureVersion = pipeline.getVersion(textureUuid);
  const std::uint64_t initialMeshVersion = pipeline.getVersion(meshUuid);
  assert(initialTextureVersion >= 1);
  assert(initialMeshVersion >= 1);

  std::this_thread::sleep_for(std::chrono::milliseconds(20));
  writeTestPpm(textureSource, 40);
  const std::size_t hotReloadScheduled = pipeline.pollHotReload();
  assert(hotReloadScheduled >= 2); // changed texture + dependent mesh

  const auto reloadStart = std::chrono::steady_clock::now();
  while (std::chrono::steady_clock::now() - reloadStart < std::chrono::seconds(2)) {
    pipeline.pollCompletedLoads();
    if (pipeline.getVersion(textureUuid) > initialTextureVersion &&
        pipeline.getVersion(meshUuid) > initialMeshVersion) {
      break;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(2));
  }

  assert(pipeline.getVersion(textureUuid) > initialTextureVersion);
  assert(pipeline.getVersion(meshUuid) > initialMeshVersion);

  std::filesystem::remove_all(tempRoot);
  return 0;
}

