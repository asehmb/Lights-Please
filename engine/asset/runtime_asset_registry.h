#pragma once

#include "../entity/AssetManager.h"
#include "../renderer/material.hpp"
#include "../renderer/mesh.h"
#include "../renderer/texture.h"
#include <memory>
#include <string>

class RuntimeAssetRegistry {
public:
  using MeshAsset = std::shared_ptr<Mesh>;
  using MaterialAsset = std::shared_ptr<Material>;
  using TextureAsset = std::shared_ptr<Texture>;

  bool addMesh(const std::string &id, MeshAsset mesh);
  bool addMaterial(const std::string &id, MaterialAsset material);
  bool addTexture(const std::string &id, TextureAsset texture);

  MeshAsset getMesh(const std::string &id) const;
  MaterialAsset getMaterial(const std::string &id) const;
  TextureAsset getTexture(const std::string &id) const;

  bool hasMesh(const std::string &id) const;
  bool hasMaterial(const std::string &id) const;
  bool hasTexture(const std::string &id) const;

private:
  AssetManager<MeshAsset> meshAssets;
  AssetManager<MaterialAsset> materialAssets;
  AssetManager<TextureAsset> textureAssets;
};
