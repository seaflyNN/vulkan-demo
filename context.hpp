#pragma once
#include <vulkan/vulkan.hpp>

//
#include <iostream>
#include <ranges>
#include <vector>

namespace helper {
inline void print_all_layers() {
  auto layers = vk::enumerateInstanceLayerProperties();
  for (const auto& layer : layers) {
    std::cout << "Layer: " << layer.layerName << "\n";
  }
}

}  // namespace helper

namespace hf {
class Context {
  vk::Instance instance;

 public:
  Context() {
    vk::ApplicationInfo app_info;
    app_info.setApiVersion(VK_API_VERSION_1_3);

    helper::print_all_layers();

    std::vector<const char*> layers{
        "VK_LAYER_KHRONOS_validation"};

    vk::InstanceCreateInfo create_info;
    create_info.setPApplicationInfo(&app_info)
        .setPEnabledLayerNames(layers);

    instance = vk::createInstance(create_info);
  }
  ~Context() {
    instance.destroy();
  }
};
};  // namespace hf