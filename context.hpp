#pragma once
#include <vulkan/vulkan.hpp>

//
#include <cassert>
#include <functional>
#include <iostream>
#include <optional>
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
    std::optional<int64_t> present_family;

    operator bool() const {
      return graphics_family.has_value() && present_family.has_value();
    }
  };

public:
  using instance_extensions_t = std::vector<const char *>;
  using create_surface_fn_t =
      std::function<vk::SurfaceKHR(const vk::Instance &)>;

public:
  vk::Instance instance;
  vk::PhysicalDevice phy_device;
  QueueFamilyIndices que_family_indices;
  vk::SurfaceKHR surface;
  vk::Device device;

public:
  Context(const instance_extensions_t &instance_extensions,
          create_surface_fn_t fn) {
    instance = create_instance(instance_extensions);
    assert(instance != VK_NULL_HANDLE);
    phy_device = pickup_physical_device(instance);
    assert(phy_device != VK_NULL_HANDLE);

    surface = fn(instance);
    assert(surface != VK_NULL_HANDLE);

    que_family_indices =
        query_family_indices(phy_device, surface, vk::QueueFlagBits::eGraphics);

    device = create_device(phy_device, que_family_indices);
    assert(device != VK_NULL_HANDLE);
    auto graphic_que =
        device.getQueue(que_family_indices.graphics_family.value(), 0);
    auto present_que =
        device.getQueue(que_family_indices.present_family.value(), 0);
  }

  ~Context() {
    device.destroy();
    if (surface != VK_NULL_HANDLE) {
      instance.destroySurfaceKHR(surface);
      surface = VK_NULL_HANDLE;
    }
    instance.destroy();
  }

  Context(const Context &) = delete;
  Context &operator=(const Context &) = delete;

private:
  static vk::Instance
  create_instance(const instance_extensions_t &instance_extensions) {
    vk::ApplicationInfo app_info;
    app_info.setApiVersion(VK_API_VERSION_1_3);
    // helper::print_all_layers();
    std::vector<const char *> layers{"VK_LAYER_KHRONOS_validation"};
    vk::InstanceCreateInfo create_info;
    create_info.setPApplicationInfo(&app_info)
        .setPEnabledLayerNames(layers)
        .setPEnabledExtensionNames(instance_extensions);
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

  static vk::Device
  create_device(const vk::PhysicalDevice &phy_device,
                const QueueFamilyIndices &que_family_indices) {
    assert(que_family_indices);

    bool is_same_family = que_family_indices.graphics_family.value() ==
                          que_family_indices.present_family.value();

    vk::DeviceCreateInfo create_info;
    std::vector<vk::DeviceQueueCreateInfo> que_create_infos;
    que_create_infos.reserve(is_same_family ? 1 : 2);

    float que_priority = 1.0f;
    vk::DeviceQueueCreateInfo graphic_que_create_info;
    graphic_que_create_info.setPQueuePriorities(&que_priority)
        .setQueueCount(1)
        .setQueueFamilyIndex(que_family_indices.graphics_family.value());
    que_create_infos.push_back(std::move(graphic_que_create_info));

    if (!is_same_family) {
      vk::DeviceQueueCreateInfo present_queue_create_info;
      present_queue_create_info.setPQueuePriorities(&que_priority)
          .setQueueCount(1)
          .setQueueFamilyIndex(que_family_indices.present_family.value());
      que_create_infos.push_back(std::move(present_queue_create_info));
    }

    create_info.setQueueCreateInfos(que_create_infos);
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

  static QueueFamilyIndices
  query_family_indices(const vk::PhysicalDevice &phy_device,
                       vk::SurfaceKHR surface, vk::QueueFlagBits flags) {
    QueueFamilyIndices indices;
    auto queue_families = phy_device.getQueueFamilyProperties();
    for (int64_t i = 0; i < static_cast<int64_t>(queue_families.size()); i++) {
      if (queue_families[i].queueFlags & flags) {
        indices.graphics_family = i;
      }
      if (phy_device.getSurfaceSupportKHR(i, surface)) {
        indices.present_family = i;
      }
      if (indices)
        break;
    }
    return indices;
  }
};

}; // namespace hf