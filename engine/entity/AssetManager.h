
#ifndef ASSET_MANAGER_H
#define ASSET_MANAGER_H

#include <iostream>
#include <string>
#include <unordered_map>

template <typename T, typename Key = std::string> class AssetManager {
public:
  AssetManager() = default;

  ~AssetManager() = default;

  // Delete copy constructor and assignment operator to prevent
  // accidental duplication of heavy resource managers
  AssetManager(const AssetManager &) = delete;
  AssetManager &operator=(const AssetManager &) = delete;

  // 1. ADD: Inserts an already constructed asset into the map
  void Add(const Key &id, const T &asset) {
    if (!Has(id)) {
      assets[id] = asset;
    } else {
      std::cout << "Warning: Asset already exists\n";
    }
  }

  T *Get(const Key &id) {
    auto it = assets.find(id);
    if (it != assets.end()) {
      // Safe to return a pointer; unordered_map doesn't invalidate
      // pointers to values when new elements are added.
      return &(it->second);
    }

    std::cerr << "Error: Asset not found\n";
    return nullptr;
  }

  const T *Get(const Key &id) const {
    auto it = assets.find(id);
    if (it != assets.end()) {
      return &(it->second);
    }

    std::cerr << "Error: Asset not found\n";
    return nullptr;
  }

  bool Has(const Key &id) const {
    return assets.find(id) != assets.end();
  }

  void Remove(const Key &id) { assets.erase(id); }

  void Clear() { assets.clear(); }

private:
  std::unordered_map<Key, T> assets;
};

// data structure to hold all assets, such as textures, models, sounds, etc.
// This will allow us to easily manage and access our assets throughout the
// game. We can use a map or unordered_map to store the assets with a string key
// for easy retrieval.

#endif // ASSET_MANAGER_H
