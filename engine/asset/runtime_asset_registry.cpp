#include "runtime_asset_registry.h"

#include "../logger.h"

namespace {
template <typename T> bool addUniqueAsset(AssetManager<T> &manager,
                                          const std::string &id,
                                          const T &asset,
                                          const char *assetType) {
  if (!asset) {
    LOG_WARN("ASSET_REGISTRY", "Rejected null {} asset '{}'", assetType, id);
    return false;
  }

  if (manager.Has(id)) {
    LOG_WARN("ASSET_REGISTRY", "{} asset '{}' already exists", assetType, id);
    return false;
  }

  manager.Add(id, asset);
  return true;
}

template <typename T>
T getAssetIfExists(const AssetManager<T> &manager, const std::string &id) {
  if (!manager.Has(id)) {
    return {};
  }

  const T *asset = manager.Get(id);
  return asset ? *asset : T{};
}
} // namespace

bool RuntimeAssetRegistry::addMesh(const std::string &id, MeshAsset mesh) {
  return addUniqueAsset(meshAssets, id, mesh, "mesh");
}

bool RuntimeAssetRegistry::addMaterial(const std::string &id,
                                       MaterialAsset material) {
  return addUniqueAsset(materialAssets, id, material, "material");
}

bool RuntimeAssetRegistry::addTexture(const std::string &id,
                                      TextureAsset texture) {
  return addUniqueAsset(textureAssets, id, texture, "texture");
}

RuntimeAssetRegistry::MeshAsset
RuntimeAssetRegistry::getMesh(const std::string &id) const {
  return getAssetIfExists(meshAssets, id);
}

RuntimeAssetRegistry::MaterialAsset
RuntimeAssetRegistry::getMaterial(const std::string &id) const {
  return getAssetIfExists(materialAssets, id);
}

RuntimeAssetRegistry::TextureAsset
RuntimeAssetRegistry::getTexture(const std::string &id) const {
  return getAssetIfExists(textureAssets, id);
}

bool RuntimeAssetRegistry::hasMesh(const std::string &id) const {
  return meshAssets.Has(id);
}

bool RuntimeAssetRegistry::hasMaterial(const std::string &id) const {
  return materialAssets.Has(id);
}

bool RuntimeAssetRegistry::hasTexture(const std::string &id) const {
  return textureAssets.Has(id);
}
