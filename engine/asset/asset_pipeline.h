#pragma once

#include "../job_system.h"
#include <array>
#include <atomic>
#include <cstdint>
#include <filesystem>
#include <limits>
#include <memory>
#include <optional>
#include <unordered_map>
#include <unordered_set>
#include <variant>
#include <vector>

namespace asset {

using AssetUUID = std::array<std::uint8_t, 16>;

struct AssetUUIDHash {
  std::size_t operator()(const AssetUUID &uuid) const noexcept;
};

AssetUUID generateUUID();
std::string assetUUIDToString(const AssetUUID &uuid);

enum class AssetType : std::uint8_t { Mesh = 1, Texture = 2 };

struct MeshAssetData {
  std::vector<float> positions;
  std::vector<float> normals;
  std::vector<float> uvs;
  std::vector<std::uint32_t> indices;
};

struct TextureAssetData {
  std::uint32_t width = 0;
  std::uint32_t height = 0;
  std::uint32_t channels = 0;
  std::vector<std::uint8_t> pixels;
};

using AssetPayload = std::variant<std::monostate, MeshAssetData, TextureAssetData>;

struct AssetRecordView {
  AssetUUID uuid{};
  AssetType type = AssetType::Mesh;
  std::filesystem::path sourcePath;
  std::filesystem::path binaryPath;
  std::vector<AssetUUID> dependencies;
  std::uint64_t version = 0;
  bool inFlight = false;
  bool hasPayload = false;
};

std::filesystem::path defaultBinaryPathFor(const std::filesystem::path &sourcePath,
                                           AssetType type);

class AssetPipeline {
public:
  explicit AssetPipeline(JobSystem *externalJobSystem = nullptr);
  ~AssetPipeline();

  AssetUUID registerAsset(AssetType type, const std::filesystem::path &sourcePath,
                          const std::filesystem::path &binaryPath = {},
                          const std::vector<AssetUUID> &dependencies = {});

  bool hasAsset(const AssetUUID &uuid) const;
  std::optional<AssetUUID>
  findBySourcePath(const std::filesystem::path &sourcePath) const;
  std::optional<AssetRecordView> getRecord(const AssetUUID &uuid) const;
  std::uint64_t getVersion(const AssetUUID &uuid) const;

  bool addDependency(const AssetUUID &asset, const AssetUUID &dependsOn);
  std::vector<AssetUUID> getDependencies(const AssetUUID &uuid) const;

  bool requestLoad(const AssetUUID &uuid);
  std::size_t pollCompletedLoads(
      std::size_t maxLoads = std::numeric_limits<std::size_t>::max());
  std::size_t pollHotReload();

  std::shared_ptr<const AssetPayload> tryGetAsset(const AssetUUID &uuid) const;
  bool isLoadInFlight(const AssetUUID &uuid) const;
  std::size_t pendingLoadCount() const;

private:
  struct AssetRecord {
    AssetUUID uuid{};
    AssetType type = AssetType::Mesh;
    std::filesystem::path sourcePath;
    std::filesystem::path binaryPath;
    std::vector<AssetUUID> dependencies;
    std::filesystem::file_time_type sourceTimestamp =
        std::filesystem::file_time_type::min();
    std::uint64_t version = 0;
    bool inFlight = false;
    std::shared_ptr<const AssetPayload> payload;
  };

  struct LoadResult {
    AssetUUID uuid{};
    bool success = false;
    std::shared_ptr<const AssetPayload> payload;
    std::filesystem::file_time_type sourceTimestamp =
        std::filesystem::file_time_type::min();
  };

  LoadResult loadAssetJob(const AssetRecord &recordSnapshot) const;
  std::shared_ptr<const AssetPayload>
  loadMeshAsset(const std::filesystem::path &sourcePath,
                const std::filesystem::path &binaryPath,
                std::filesystem::file_time_type &sourceTimestampOut) const;
  std::shared_ptr<const AssetPayload>
  loadTextureAsset(const std::filesystem::path &sourcePath,
                   const std::filesystem::path &binaryPath,
                   std::filesystem::file_time_type &sourceTimestampOut) const;

  static std::filesystem::path normalizePath(const std::filesystem::path &path);
  static bool isBinaryUpToDate(const std::filesystem::path &sourcePath,
                               const std::filesystem::path &binaryPath);

  class Impl;
  std::unique_ptr<Impl> impl;
};

} // namespace asset

