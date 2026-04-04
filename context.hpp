#pragma once
#include <vulkan/vulkan.hpp>

//
#include <cassert>
#include <iostream>
#include <ranges>
#include <vector>

namespace helper {
inline void print_all_layers() {
  auto layers = vk::enumerateInstanceLayerProperties();
  for (const auto &layer : layers) {
    std::cout << "Layer: " << layer.layerName << "\n";
  }
}

} // namespace helper

namespace hf {
class Context {
  struct QueueFamilyIndices {
    std::optional<int64_t> graphics_family;
  };

public:
  vk::Instance instance;
  vk::PhysicalDevice phy_device;
  QueueFamilyIndices que_family_indices;
  vk::Device device;

public:
  Context() {
    instance = create_instance();
    assert(instance != VK_NULL_HANDLE);
    phy_device = pickup_physical_device(instance);
    assert(phy_device != VK_NULL_HANDLE);
    que_family_indices =
        query_family_indices(phy_device, vk::QueueFlagBits::eGraphics);
    assert(que_family_indices.graphics_family.has_value());
    device = create_device(phy_device);
    assert(device != VK_NULL_HANDLE);
    auto graphic_que =
        device.getQueue(que_family_indices.graphics_family.value(), 0);
  }

  ~Context() {
    device.destroy();
    instance.destroy();
  }

  Context(const Context &) = delete;
  Context &operator=(const Context &) = delete;

private:
  static vk::Instance create_instance() {
    vk::ApplicationInfo app_info;
    app_info.setApiVersion(VK_API_VERSION_1_3);
    // helper::print_all_layers();
    std::vector<const char *> layers{"VK_LAYER_KHRONOS_validation"};
    vk::InstanceCreateInfo create_info;
    create_info.setPApplicationInfo(&app_info).setPEnabledLayerNames(layers);
    return vk::createInstance(create_info);
  }

  static vk::PhysicalDevice
  pickup_physical_device(const vk::Instance &instance) {
    auto devices = instance.enumeratePhysicalDevices();
    for (const auto &device : devices) {
      auto properties = device.getProperties();
      std::cout << "Device: " << properties.deviceName << "\n";
    }

    return devices[0];
  }

  static vk::Device create_device(const vk::PhysicalDevice &phy_device) {
    vk::DeviceCreateInfo create_info;
    vk::DeviceQueueCreateInfo que_create_info;
    float que_priority = 1.0f;

    auto graphic_family_indices =
        query_family_indices(phy_device, vk::QueueFlagBits::eGraphics);
    assert(graphic_family_indices.graphics_family.has_value());
    que_create_info.setPQueuePriorities(&que_priority)
        .setQueueCount(1)
        .setQueueFamilyIndex(graphic_family_indices.graphics_family.value());

    create_info.setQueueCreateInfos(que_create_info);
    return phy_device.createDevice(create_info);
  }

  static QueueFamilyIndices
  query_family_indices(const vk::PhysicalDevice &phy_device,
                       vk::QueueFlagBits flags) {
    QueueFamilyIndices indices;
    auto queue_families = phy_device.getQueueFamilyProperties();
    for (int64_t i = 0; i < static_cast<int64_t>(queue_families.size()); i++) {
      if (queue_families[i].queueFlags & flags) {
        indices.graphics_family = i;
        break;
      }
    }
    return indices;
  }
};

}; // namespace hf