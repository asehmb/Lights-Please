#include "asset_pipeline.h"

#include "../logger.h"
#include <algorithm>
#include <cctype>
#include <chrono>
#include <deque>
#include <fstream>
#include <queue>
#include <random>
#include <sstream>
#include <stdexcept>
#include <string>
#include <thread>

namespace asset {
namespace {

constexpr std::uint32_t MESH_BINARY_MAGIC = 0x4C4D5348;    // LMSH
constexpr std::uint32_t TEXTURE_BINARY_MAGIC = 0x4C544558; // LTEX
constexpr std::uint32_t BINARY_VERSION = 1;

struct MeshBinaryHeader {
  std::uint32_t magic = MESH_BINARY_MAGIC;
  std::uint32_t version = BINARY_VERSION;
  std::uint64_t positionCount = 0;
  std::uint64_t normalCount = 0;
  std::uint64_t uvCount = 0;
  std::uint64_t indexCount = 0;
};

struct TextureBinaryHeader {
  std::uint32_t magic = TEXTURE_BINARY_MAGIC;
  std::uint32_t version = BINARY_VERSION;
  std::uint32_t width = 0;
  std::uint32_t height = 0;
  std::uint32_t channels = 0;
  std::uint64_t pixelCount = 0;
};

struct ObjVertexKey {
  int positionIndex = -1;
  int uvIndex = -1;
  int normalIndex = -1;

  bool operator==(const ObjVertexKey &other) const {
    return positionIndex == other.positionIndex && uvIndex == other.uvIndex &&
           normalIndex == other.normalIndex;
  }
};

struct ObjVertexKeyHash {
  std::size_t operator()(const ObjVertexKey &key) const noexcept {
    std::size_t h = std::hash<int>{}(key.positionIndex);
    h ^= std::hash<int>{}(key.uvIndex) + 0x9e3779b97f4a7c15ULL + (h << 6) +
         (h >> 2);
    h ^= std::hash<int>{}(key.normalIndex) + 0x9e3779b97f4a7c15ULL + (h << 6) +
         (h >> 2);
    return h;
  }
};

int resolveObjIndex(int rawIndex, int count) {
  if (rawIndex > 0) {
    return rawIndex - 1;
  }
  if (rawIndex < 0) {
    return count + rawIndex;
  }
  return -1;
}

bool readNextTokenSkippingComments(std::istream &stream, std::string &token) {
  while (stream >> token) {
    if (!token.empty() && token[0] == '#') {
      std::string ignored;
      std::getline(stream, ignored);
      continue;
    }
    return true;
  }
  return false;
}

ObjVertexKey parseObjFaceToken(const std::string &token, int positionCount,
                               int uvCount, int normalCount) {
  ObjVertexKey parsed{};

  const std::size_t firstSlash = token.find('/');
  if (firstSlash == std::string::npos) {
    parsed.positionIndex = resolveObjIndex(std::stoi(token), positionCount);
    return parsed;
  }

  const std::string positionToken = token.substr(0, firstSlash);
  if (!positionToken.empty()) {
    parsed.positionIndex = resolveObjIndex(std::stoi(positionToken), positionCount);
  }

  const std::size_t secondSlash = token.find('/', firstSlash + 1);
  if (secondSlash == std::string::npos) {
    const std::string uvToken = token.substr(firstSlash + 1);
    if (!uvToken.empty()) {
      parsed.uvIndex = resolveObjIndex(std::stoi(uvToken), uvCount);
    }
    return parsed;
  }

  const std::string uvToken = token.substr(firstSlash + 1, secondSlash - firstSlash - 1);
  if (!uvToken.empty()) {
    parsed.uvIndex = resolveObjIndex(std::stoi(uvToken), uvCount);
  }

  const std::string normalToken = token.substr(secondSlash + 1);
  if (!normalToken.empty()) {
    parsed.normalIndex = resolveObjIndex(std::stoi(normalToken), normalCount);
  }

  return parsed;
}

MeshAssetData importObjMesh(const std::filesystem::path &sourcePath) {
  std::ifstream in(sourcePath);
  if (!in) {
    throw std::runtime_error("Failed to open mesh source: " + sourcePath.string());
  }

  std::vector<std::array<float, 3>> positions;
  std::vector<std::array<float, 3>> normals;
  std::vector<std::array<float, 2>> uvs;
  std::unordered_map<ObjVertexKey, std::uint32_t, ObjVertexKeyHash> uniqueVertices;
  MeshAssetData meshData;

  std::string line;
  while (std::getline(in, line)) {
    if (line.empty() || line[0] == '#') {
      continue;
    }

    std::istringstream lineStream(line);
    std::string prefix;
    lineStream >> prefix;

    if (prefix == "v") {
      std::array<float, 3> pos{};
      lineStream >> pos[0] >> pos[1] >> pos[2];
      positions.push_back(pos);
      continue;
    }
    if (prefix == "vn") {
      std::array<float, 3> normal{};
      lineStream >> normal[0] >> normal[1] >> normal[2];
      normals.push_back(normal);
      continue;
    }
    if (prefix == "vt") {
      std::array<float, 2> uv{};
      lineStream >> uv[0] >> uv[1];
      uvs.push_back(uv);
      continue;
    }
    if (prefix != "f") {
      continue;
    }

    std::vector<std::uint32_t> faceVertexIndices;
    std::string vertexToken;
    while (lineStream >> vertexToken) {
      const ObjVertexKey key =
          parseObjFaceToken(vertexToken, static_cast<int>(positions.size()),
                            static_cast<int>(uvs.size()),
                            static_cast<int>(normals.size()));
      if (key.positionIndex < 0 ||
          key.positionIndex >= static_cast<int>(positions.size())) {
        throw std::runtime_error("OBJ face references invalid position index");
      }

      auto existing = uniqueVertices.find(key);
      if (existing == uniqueVertices.end()) {
        const std::uint32_t newIndex =
            static_cast<std::uint32_t>(meshData.positions.size() / 3);
        uniqueVertices.emplace(key, newIndex);

        const auto &pos = positions[static_cast<std::size_t>(key.positionIndex)];
        meshData.positions.insert(meshData.positions.end(),
                                  {pos[0], pos[1], pos[2]});

        if (key.normalIndex >= 0 &&
            key.normalIndex < static_cast<int>(normals.size())) {
          const auto &normal =
              normals[static_cast<std::size_t>(key.normalIndex)];
          meshData.normals.insert(meshData.normals.end(),
                                  {normal[0], normal[1], normal[2]});
        } else {
          meshData.normals.insert(meshData.normals.end(), {0.0F, 0.0F, 0.0F});
        }

        if (key.uvIndex >= 0 && key.uvIndex < static_cast<int>(uvs.size())) {
          const auto &uv = uvs[static_cast<std::size_t>(key.uvIndex)];
          meshData.uvs.insert(meshData.uvs.end(), {uv[0], uv[1]});
        } else {
          meshData.uvs.insert(meshData.uvs.end(), {0.0F, 0.0F});
        }

        faceVertexIndices.push_back(newIndex);
      } else {
        faceVertexIndices.push_back(existing->second);
      }
    }

    if (faceVertexIndices.size() < 3) {
      continue;
    }

    for (std::size_t i = 1; i + 1 < faceVertexIndices.size(); ++i) {
      meshData.indices.push_back(faceVertexIndices[0]);
      meshData.indices.push_back(faceVertexIndices[i]);
      meshData.indices.push_back(faceVertexIndices[i + 1]);
    }
  }

  if (meshData.indices.empty() || meshData.positions.empty()) {
    throw std::runtime_error("Mesh importer produced empty data: " +
                             sourcePath.string());
  }

  return meshData;
}

TextureAssetData importPpmTexture(const std::filesystem::path &sourcePath) {
  std::ifstream in(sourcePath, std::ios::binary);
  if (!in) {
    throw std::runtime_error("Failed to open texture source: " +
                             sourcePath.string());
  }

  std::string magic;
  if (!readNextTokenSkippingComments(in, magic)) {
    throw std::runtime_error("Missing PPM header");
  }
  if (magic != "P6" && magic != "P3") {
    throw std::runtime_error("Unsupported texture format: expected P6 or P3 PPM");
  }

  std::string widthToken;
  std::string heightToken;
  std::string maxValueToken;
  if (!readNextTokenSkippingComments(in, widthToken) ||
      !readNextTokenSkippingComments(in, heightToken) ||
      !readNextTokenSkippingComments(in, maxValueToken)) {
    throw std::runtime_error("Invalid PPM header fields");
  }

  TextureAssetData textureData{};
  textureData.width = static_cast<std::uint32_t>(std::stoul(widthToken));
  textureData.height = static_cast<std::uint32_t>(std::stoul(heightToken));
  const int maxValue = std::stoi(maxValueToken);
  if (maxValue <= 0 || maxValue > 255) {
    throw std::runtime_error("Unsupported PPM max value");
  }

  textureData.channels = 3;
  const std::size_t pixelCount =
      static_cast<std::size_t>(textureData.width) * textureData.height *
      textureData.channels;
  textureData.pixels.resize(pixelCount);

  if (magic == "P6") {
    char separator = '\0';
    in.get(separator);
    if (!in || !std::isspace(static_cast<unsigned char>(separator))) {
      throw std::runtime_error("Invalid P6 separator after max value");
    }
    in.read(reinterpret_cast<char *>(textureData.pixels.data()),
            static_cast<std::streamsize>(textureData.pixels.size()));
    if (in.gcount() != static_cast<std::streamsize>(textureData.pixels.size())) {
      throw std::runtime_error("Unexpected EOF while reading P6 texture payload");
    }
  } else {
    for (std::size_t i = 0; i < textureData.pixels.size(); ++i) {
      std::string componentToken;
      if (!readNextTokenSkippingComments(in, componentToken)) {
        throw std::runtime_error("Unexpected EOF while reading P3 texture payload");
      }
      const int value = std::stoi(componentToken);
      if (value < 0 || value > maxValue) {
        throw std::runtime_error("PPM pixel component out of range");
      }
      textureData.pixels[i] =
          static_cast<std::uint8_t>((value * 255) / maxValue);
    }
  }

  return textureData;
}

void writeMeshBinary(const std::filesystem::path &binaryPath,
                     const MeshAssetData &meshData) {
  std::filesystem::create_directories(binaryPath.parent_path());
  std::ofstream out(binaryPath, std::ios::binary | std::ios::trunc);
  if (!out) {
    throw std::runtime_error("Failed to open mesh binary for writing: " +
                             binaryPath.string());
  }

  MeshBinaryHeader header{};
  header.positionCount = meshData.positions.size();
  header.normalCount = meshData.normals.size();
  header.uvCount = meshData.uvs.size();
  header.indexCount = meshData.indices.size();
  out.write(reinterpret_cast<const char *>(&header), sizeof(header));

  out.write(reinterpret_cast<const char *>(meshData.positions.data()),
            static_cast<std::streamsize>(meshData.positions.size() *
                                         sizeof(float)));
  out.write(reinterpret_cast<const char *>(meshData.normals.data()),
            static_cast<std::streamsize>(meshData.normals.size() *
                                         sizeof(float)));
  out.write(reinterpret_cast<const char *>(meshData.uvs.data()),
            static_cast<std::streamsize>(meshData.uvs.size() *
                                         sizeof(float)));
  out.write(reinterpret_cast<const char *>(meshData.indices.data()),
            static_cast<std::streamsize>(meshData.indices.size() *
                                         sizeof(std::uint32_t)));

  if (!out) {
    throw std::runtime_error("Failed while writing mesh binary: " +
                             binaryPath.string());
  }
}

MeshAssetData readMeshBinary(const std::filesystem::path &binaryPath) {
  std::ifstream in(binaryPath, std::ios::binary);
  if (!in) {
    throw std::runtime_error("Failed to open mesh binary: " + binaryPath.string());
  }

  MeshBinaryHeader header{};
  in.read(reinterpret_cast<char *>(&header), sizeof(header));
  if (!in || header.magic != MESH_BINARY_MAGIC ||
      header.version != BINARY_VERSION) {
    throw std::runtime_error("Invalid mesh binary header: " + binaryPath.string());
  }

  MeshAssetData meshData{};
  meshData.positions.resize(static_cast<std::size_t>(header.positionCount));
  meshData.normals.resize(static_cast<std::size_t>(header.normalCount));
  meshData.uvs.resize(static_cast<std::size_t>(header.uvCount));
  meshData.indices.resize(static_cast<std::size_t>(header.indexCount));

  in.read(reinterpret_cast<char *>(meshData.positions.data()),
          static_cast<std::streamsize>(meshData.positions.size() * sizeof(float)));
  in.read(reinterpret_cast<char *>(meshData.normals.data()),
          static_cast<std::streamsize>(meshData.normals.size() * sizeof(float)));
  in.read(reinterpret_cast<char *>(meshData.uvs.data()),
          static_cast<std::streamsize>(meshData.uvs.size() * sizeof(float)));
  in.read(reinterpret_cast<char *>(meshData.indices.data()),
          static_cast<std::streamsize>(meshData.indices.size() *
                                       sizeof(std::uint32_t)));
  if (!in) {
    throw std::runtime_error("Corrupt mesh binary payload: " + binaryPath.string());
  }

  return meshData;
}

void writeTextureBinary(const std::filesystem::path &binaryPath,
                        const TextureAssetData &textureData) {
  std::filesystem::create_directories(binaryPath.parent_path());
  std::ofstream out(binaryPath, std::ios::binary | std::ios::trunc);
  if (!out) {
    throw std::runtime_error("Failed to open texture binary for writing: " +
                             binaryPath.string());
  }

  TextureBinaryHeader header{};
  header.width = textureData.width;
  header.height = textureData.height;
  header.channels = textureData.channels;
  header.pixelCount = textureData.pixels.size();
  out.write(reinterpret_cast<const char *>(&header), sizeof(header));
  out.write(reinterpret_cast<const char *>(textureData.pixels.data()),
            static_cast<std::streamsize>(textureData.pixels.size()));
  if (!out) {
    throw std::runtime_error("Failed while writing texture binary: " +
                             binaryPath.string());
  }
}

TextureAssetData readTextureBinary(const std::filesystem::path &binaryPath) {
  std::ifstream in(binaryPath, std::ios::binary);
  if (!in) {
    throw std::runtime_error("Failed to open texture binary: " +
                             binaryPath.string());
  }

  TextureBinaryHeader header{};
  in.read(reinterpret_cast<char *>(&header), sizeof(header));
  if (!in || header.magic != TEXTURE_BINARY_MAGIC ||
      header.version != BINARY_VERSION) {
    throw std::runtime_error("Invalid texture binary header: " +
                             binaryPath.string());
  }

  TextureAssetData textureData{};
  textureData.width = header.width;
  textureData.height = header.height;
  textureData.channels = header.channels;
  textureData.pixels.resize(static_cast<std::size_t>(header.pixelCount));
  in.read(reinterpret_cast<char *>(textureData.pixels.data()),
          static_cast<std::streamsize>(textureData.pixels.size()));
  if (!in) {
    throw std::runtime_error("Corrupt texture binary payload: " +
                             binaryPath.string());
  }
  return textureData;
}

} // namespace

class AssetPipeline::Impl {
public:
  explicit Impl(JobSystem *externalJobSystem)
      : jobSystem(externalJobSystem), pendingLoads(0) {
    if (!jobSystem) {
      ownedJobSystem = std::make_unique<JobSystem>();
      ownedJobSystem->initialize(0);
      jobSystem = ownedJobSystem.get();
    }
  }

  std::unique_ptr<JobSystem> ownedJobSystem;
  JobSystem *jobSystem = nullptr;

  mutable std::mutex recordsMutex;
  std::unordered_map<AssetUUID, AssetRecord, AssetUUIDHash> recordsByUuid;
  std::unordered_map<std::string, AssetUUID> uuidBySourcePath;
  std::unordered_map<AssetUUID, std::unordered_set<AssetUUID, AssetUUIDHash>,
                     AssetUUIDHash>
      dependents;

  mutable std::mutex completionsMutex;
  std::deque<LoadResult> completedLoads;
  std::atomic<std::size_t> pendingLoads;
};

std::size_t AssetUUIDHash::operator()(const AssetUUID &uuid) const noexcept {
  std::size_t hash = 1469598103934665603ULL;
  for (std::uint8_t byte : uuid) {
    hash ^= static_cast<std::size_t>(byte);
    hash *= 1099511628211ULL;
  }
  return hash;
}

AssetUUID generateUUID() {
  static thread_local std::mt19937_64 rng{std::random_device{}()};
  std::uniform_int_distribution<std::uint32_t> dist(0, 255);

  AssetUUID uuid{};
  for (auto &byte : uuid) {
    byte = static_cast<std::uint8_t>(dist(rng));
  }

  // UUID v4 semantics
  uuid[6] = static_cast<std::uint8_t>((uuid[6] & 0x0F) | 0x40);
  uuid[8] = static_cast<std::uint8_t>((uuid[8] & 0x3F) | 0x80);
  return uuid;
}

std::string assetUUIDToString(const AssetUUID &uuid) {
  static constexpr char hex[] = "0123456789abcdef";
  std::string value;
  value.reserve(36);
  for (std::size_t i = 0; i < uuid.size(); ++i) {
    if (i == 4 || i == 6 || i == 8 || i == 10) {
      value.push_back('-');
    }
    const std::uint8_t byte = uuid[i];
    value.push_back(hex[(byte >> 4) & 0x0F]);
    value.push_back(hex[byte & 0x0F]);
  }
  return value;
}

std::filesystem::path
defaultBinaryPathFor(const std::filesystem::path &sourcePath, AssetType type) {
  std::filesystem::path binary = sourcePath;
  if (type == AssetType::Mesh) {
    binary.replace_extension(".lmesh");
  } else {
    binary.replace_extension(".ltex");
  }
  return binary;
}

AssetPipeline::AssetPipeline(JobSystem *externalJobSystem)
    : impl(std::make_unique<Impl>(externalJobSystem)) {}

AssetPipeline::~AssetPipeline() {
  while (impl->pendingLoads.load(std::memory_order_acquire) > 0) {
    pollCompletedLoads();
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }
  pollCompletedLoads();
}

std::filesystem::path
AssetPipeline::normalizePath(const std::filesystem::path &path) {
  if (path.empty()) {
    return {};
  }
  std::error_code ec;
  if (std::filesystem::exists(path, ec)) {
    return std::filesystem::weakly_canonical(path, ec);
  }
  return std::filesystem::absolute(path, ec).lexically_normal();
}

AssetUUID AssetPipeline::registerAsset(AssetType type,
                                       const std::filesystem::path &sourcePath,
                                       const std::filesystem::path &binaryPath,
                                       const std::vector<AssetUUID> &dependencies) {
  AssetRecord record{};
  record.type = type;
  record.sourcePath = normalizePath(sourcePath);
  record.binaryPath = binaryPath.empty()
                          ? defaultBinaryPathFor(record.sourcePath, type)
                          : normalizePath(binaryPath);
  if (std::filesystem::exists(record.sourcePath)) {
    record.sourceTimestamp = std::filesystem::last_write_time(record.sourcePath);
  }

  {
    std::lock_guard<std::mutex> lock(impl->recordsMutex);
    do {
      record.uuid = generateUUID();
    } while (impl->recordsByUuid.contains(record.uuid));

    impl->uuidBySourcePath[record.sourcePath.string()] = record.uuid;
    impl->recordsByUuid.emplace(record.uuid, record);
  }

  for (const AssetUUID &dependency : dependencies) {
    addDependency(record.uuid, dependency);
  }

  LOG_INFO("ASSET", "Registered {} [{}]", record.sourcePath.string(),
           assetUUIDToString(record.uuid));
  return record.uuid;
}

bool AssetPipeline::hasAsset(const AssetUUID &uuid) const {
  std::lock_guard<std::mutex> lock(impl->recordsMutex);
  return impl->recordsByUuid.contains(uuid);
}

std::optional<AssetUUID>
AssetPipeline::findBySourcePath(const std::filesystem::path &sourcePath) const {
  const std::string normalized = normalizePath(sourcePath).string();
  std::lock_guard<std::mutex> lock(impl->recordsMutex);
  const auto it = impl->uuidBySourcePath.find(normalized);
  if (it == impl->uuidBySourcePath.end()) {
    return std::nullopt;
  }
  return it->second;
}

std::optional<AssetRecordView> AssetPipeline::getRecord(const AssetUUID &uuid) const {
  std::lock_guard<std::mutex> lock(impl->recordsMutex);
  const auto it = impl->recordsByUuid.find(uuid);
  if (it == impl->recordsByUuid.end()) {
    return std::nullopt;
  }

  const AssetRecord &record = it->second;
  AssetRecordView view{};
  view.uuid = record.uuid;
  view.type = record.type;
  view.sourcePath = record.sourcePath;
  view.binaryPath = record.binaryPath;
  view.dependencies = record.dependencies;
  view.version = record.version;
  view.inFlight = record.inFlight;
  view.hasPayload = static_cast<bool>(record.payload);
  return view;
}

std::uint64_t AssetPipeline::getVersion(const AssetUUID &uuid) const {
  std::lock_guard<std::mutex> lock(impl->recordsMutex);
  const auto it = impl->recordsByUuid.find(uuid);
  if (it == impl->recordsByUuid.end()) {
    return 0;
  }
  return it->second.version;
}

bool AssetPipeline::addDependency(const AssetUUID &asset,
                                  const AssetUUID &dependsOn) {
  std::lock_guard<std::mutex> lock(impl->recordsMutex);
  auto assetIt = impl->recordsByUuid.find(asset);
  if (assetIt == impl->recordsByUuid.end()) {
    return false;
  }

  auto &dependencies = assetIt->second.dependencies;
  if (std::find(dependencies.begin(), dependencies.end(), dependsOn) ==
      dependencies.end()) {
    dependencies.push_back(dependsOn);
  }
  impl->dependents[dependsOn].insert(asset);
  return true;
}

std::vector<AssetUUID> AssetPipeline::getDependencies(const AssetUUID &uuid) const {
  std::lock_guard<std::mutex> lock(impl->recordsMutex);
  const auto it = impl->recordsByUuid.find(uuid);
  if (it == impl->recordsByUuid.end()) {
    return {};
  }
  return it->second.dependencies;
}

bool AssetPipeline::requestLoad(const AssetUUID &uuid) {
  AssetRecord snapshot{};
  {
    std::lock_guard<std::mutex> lock(impl->recordsMutex);
    const auto it = impl->recordsByUuid.find(uuid);
    if (it == impl->recordsByUuid.end() || it->second.inFlight) {
      return false;
    }
    it->second.inFlight = true;
    snapshot = it->second;
  }

  impl->pendingLoads.fetch_add(1, std::memory_order_acq_rel);
  impl->jobSystem->kickJob(
      [this, snapshot]() mutable {
        LoadResult result{};
        try {
          result = loadAssetJob(snapshot);
        } catch (const std::exception &e) {
          LOG_ERR("ASSET", "Load failed for {}: {}", snapshot.sourcePath.string(),
                  e.what());
          result.uuid = snapshot.uuid;
          result.success = false;
        }

        {
          std::lock_guard<std::mutex> lock(impl->completionsMutex);
          impl->completedLoads.push_back(std::move(result));
        }
        impl->pendingLoads.fetch_sub(1, std::memory_order_acq_rel);
      },
      nullptr);
  return true;
}

std::size_t AssetPipeline::pollCompletedLoads(std::size_t maxLoads) {
  std::size_t applied = 0;
  while (applied < maxLoads) {
    LoadResult result{};
    {
      std::lock_guard<std::mutex> lock(impl->completionsMutex);
      if (impl->completedLoads.empty()) {
        break;
      }
      result = std::move(impl->completedLoads.front());
      impl->completedLoads.pop_front();
    }

    std::lock_guard<std::mutex> lock(impl->recordsMutex);
    const auto it = impl->recordsByUuid.find(result.uuid);
    if (it == impl->recordsByUuid.end()) {
      continue;
    }

    AssetRecord &record = it->second;
    record.inFlight = false;
    if (result.success) {
      record.payload = std::move(result.payload);
      record.sourceTimestamp = result.sourceTimestamp;
      record.version += 1;
      LOG_INFO("ASSET", "Loaded {} version {}", record.sourcePath.string(),
               record.version);
    }

    ++applied;
  }
  return applied;
}

std::size_t AssetPipeline::pollHotReload() {
  std::vector<AssetUUID> changedRoots;

  {
    std::lock_guard<std::mutex> lock(impl->recordsMutex);
    for (const auto &[uuid, record] : impl->recordsByUuid) {
      if (record.sourcePath.empty()) {
        continue;
      }
      std::error_code ec;
      if (!std::filesystem::exists(record.sourcePath, ec)) {
        continue;
      }
      const auto currentTimestamp =
          std::filesystem::last_write_time(record.sourcePath, ec);
      if (ec) {
        continue;
      }
      if (currentTimestamp > record.sourceTimestamp) {
        changedRoots.push_back(uuid);
      }
    }
  }

  if (changedRoots.empty()) {
    return 0;
  }

  std::unordered_set<AssetUUID, AssetUUIDHash> toReload(changedRoots.begin(),
                                                        changedRoots.end());
  {
    std::lock_guard<std::mutex> lock(impl->recordsMutex);
    std::queue<AssetUUID> queue;
    for (const AssetUUID &uuid : changedRoots) {
      queue.push(uuid);
    }

    while (!queue.empty()) {
      const AssetUUID current = queue.front();
      queue.pop();
      const auto dependentsIt = impl->dependents.find(current);
      if (dependentsIt == impl->dependents.end()) {
        continue;
      }
      for (const AssetUUID &dependent : dependentsIt->second) {
        if (toReload.insert(dependent).second) {
          queue.push(dependent);
        }
      }
    }
  }

  std::size_t scheduled = 0;
  for (const AssetUUID &uuid : toReload) {
    if (requestLoad(uuid)) {
      ++scheduled;
    }
  }
  return scheduled;
}

std::shared_ptr<const AssetPayload>
AssetPipeline::tryGetAsset(const AssetUUID &uuid) const {
  std::lock_guard<std::mutex> lock(impl->recordsMutex);
  const auto it = impl->recordsByUuid.find(uuid);
  if (it == impl->recordsByUuid.end()) {
    return nullptr;
  }
  return it->second.payload;
}

bool AssetPipeline::isLoadInFlight(const AssetUUID &uuid) const {
  std::lock_guard<std::mutex> lock(impl->recordsMutex);
  const auto it = impl->recordsByUuid.find(uuid);
  return it != impl->recordsByUuid.end() && it->second.inFlight;
}

std::size_t AssetPipeline::pendingLoadCount() const {
  return impl->pendingLoads.load(std::memory_order_acquire);
}

AssetPipeline::LoadResult
AssetPipeline::loadAssetJob(const AssetRecord &recordSnapshot) const {
  LoadResult result{};
  result.uuid = recordSnapshot.uuid;
  result.success = false;

  std::filesystem::file_time_type sourceTimestamp =
      std::filesystem::file_time_type::min();
  std::shared_ptr<const AssetPayload> payload;

  if (recordSnapshot.type == AssetType::Mesh) {
    payload = loadMeshAsset(recordSnapshot.sourcePath, recordSnapshot.binaryPath,
                            sourceTimestamp);
  } else {
    payload = loadTextureAsset(recordSnapshot.sourcePath,
                               recordSnapshot.binaryPath, sourceTimestamp);
  }

  result.payload = std::move(payload);
  result.sourceTimestamp = sourceTimestamp;
  result.success = true;
  return result;
}

bool AssetPipeline::isBinaryUpToDate(const std::filesystem::path &sourcePath,
                                     const std::filesystem::path &binaryPath) {
  std::error_code ec;
  if (!std::filesystem::exists(binaryPath, ec)) {
    return false;
  }
  if (!std::filesystem::exists(sourcePath, ec)) {
    return true;
  }

  const auto sourceTime = std::filesystem::last_write_time(sourcePath, ec);
  if (ec) {
    return false;
  }
  const auto binaryTime = std::filesystem::last_write_time(binaryPath, ec);
  if (ec) {
    return false;
  }
  return binaryTime >= sourceTime;
}

std::shared_ptr<const AssetPayload> AssetPipeline::loadMeshAsset(
    const std::filesystem::path &sourcePath, const std::filesystem::path &binaryPath,
    std::filesystem::file_time_type &sourceTimestampOut) const {
  std::error_code ec;
  if (std::filesystem::exists(sourcePath, ec)) {
    sourceTimestampOut = std::filesystem::last_write_time(sourcePath, ec);
  }

  MeshAssetData meshData{};
  if (isBinaryUpToDate(sourcePath, binaryPath)) {
    meshData = readMeshBinary(binaryPath);
  } else {
    meshData = importObjMesh(sourcePath);
    writeMeshBinary(binaryPath, meshData);
  }

  return std::make_shared<const AssetPayload>(std::move(meshData));
}

std::shared_ptr<const AssetPayload> AssetPipeline::loadTextureAsset(
    const std::filesystem::path &sourcePath,
    const std::filesystem::path &binaryPath,
    std::filesystem::file_time_type &sourceTimestampOut) const {
  std::error_code ec;
  if (std::filesystem::exists(sourcePath, ec)) {
    sourceTimestampOut = std::filesystem::last_write_time(sourcePath, ec);
  }

  TextureAssetData textureData{};
  if (isBinaryUpToDate(sourcePath, binaryPath)) {
    textureData = readTextureBinary(binaryPath);
  } else {
    textureData = importPpmTexture(sourcePath);
    writeTextureBinary(binaryPath, textureData);
  }

  return std::make_shared<const AssetPayload>(std::move(textureData));
}

} // namespace asset
