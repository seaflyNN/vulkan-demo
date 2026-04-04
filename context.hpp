#pragma once

#include <vulkan/vulkan.hpp>

//
#include <algorithm>
#include <array>
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

struct QueueFamilyIndices {
  std::optional<uint32_t> graphics_family;
  std::optional<uint32_t> present_family;

  operator bool() const {
    return graphics_family.has_value() && present_family.has_value();
  }
};

class SwapChain {
  vk::Device device;
  vk::SwapchainKHR swapchain;

public:
  struct SwapChainInfo {
    vk::SurfaceFormatKHR format;
    vk::Extent2D extent;
    vk::SurfaceTransformFlagBitsKHR transform;
    vk::PresentModeKHR present_mode;
    uint32_t image_count;
  };

public:
  SwapChain() {}

  SwapChain(vk::PhysicalDevice phy_device, vk::Device device,
            vk::SurfaceKHR surface, QueueFamilyIndices indices, int w, int h)
      : device(device) {
    vk::SwapchainCreateInfoKHR create_info;

    auto swapchain_info = query_swapchain_support(
        phy_device, surface,
        vk::SurfaceFormatKHR{vk::Format::eB8G8R8A8Srgb,
                             vk::ColorSpaceKHR::eSrgbNonlinear},
        /*w=*/w, /*h=*/h);

    create_info
        .setClipped(true)
        // 图像数组的层数, 3D纹理或立方体贴图可能需要大于1
        .setImageArrayLayers(1)
        // 图像的用途, 这里是作为颜色附件
        .setImageUsage(vk::ImageUsageFlagBits::eColorAttachment)
        .setCompositeAlpha(vk::CompositeAlphaFlagBitsKHR::eOpaque)
        .setSurface(surface)
        .setImageColorSpace(swapchain_info.format.colorSpace)
        .setImageFormat(swapchain_info.format.format)
        .setImageExtent(swapchain_info.extent)
        .setMinImageCount(swapchain_info.image_count)
        .setPresentMode(swapchain_info.present_mode);

    assert(indices);
    if (indices.graphics_family.value() == indices.present_family.value()) {
      create_info.setQueueFamilyIndices(indices.graphics_family.value())
          .setImageSharingMode(vk::SharingMode::eExclusive);
    } else {
      std::array<uint32_t, 2> queue_family_indices{
          indices.graphics_family.value(), indices.present_family.value()};
      create_info.setQueueFamilyIndices(queue_family_indices)
          .setImageSharingMode(vk::SharingMode::eConcurrent);
    }

    swapchain = device.createSwapchainKHR(create_info);
  }

  ~SwapChain() {
    if (*this)
      device.destroySwapchainKHR(swapchain);
  }
  SwapChain(const SwapChain &) = delete;
  SwapChain &operator=(const SwapChain &) = delete;
  SwapChain(SwapChain &&rhs) noexcept
      : device(std::exchange(rhs.device, {})),
        swapchain(std::exchange(rhs.swapchain, {})) {}
  SwapChain &operator=(SwapChain &&rhs) noexcept {
    if (this != &rhs) {
      if (*this)
        device.destroySwapchainKHR(swapchain);

      device = std::exchange(rhs.device, {});
      swapchain = std::exchange(rhs.swapchain, {});
    }
    return *this;
  }

  operator bool() const { return swapchain && device; }

public:
  static SwapChainInfo query_swapchain_support(
      const vk::PhysicalDevice &phy_device, vk::SurfaceKHR surface,
      const vk::SurfaceFormatKHR &expect_format, int w, int h) {
    auto capabilities = phy_device.getSurfaceCapabilitiesKHR(surface);
    auto present_modes = phy_device.getSurfacePresentModesKHR(surface);
    auto formats = phy_device.getSurfaceFormatsKHR(surface);

    SwapChainInfo info;
    for (auto &&format : formats) {
      if (format == expect_format) {
        info.format = format;
        break;
      }
    }
    info.image_count = std::clamp<uint32_t>(2, capabilities.minImageCount,
                                            capabilities.maxImageCount);
    info.extent.width =
        std::clamp<uint32_t>(w, capabilities.minImageExtent.width,
                             capabilities.maxImageExtent.width);
    info.extent.height =
        std::clamp<uint32_t>(h, capabilities.minImageExtent.height,
                             capabilities.maxImageExtent.height);
    // 变换
    info.transform = capabilities.currentTransform;

    info.present_mode = vk::PresentModeKHR::eFifo;
    for (const auto &mode : present_modes) {
      if (mode == vk::PresentModeKHR::eMailbox) {
        info.present_mode = mode;
        break;
      }
    }

    return info;
  }
};

class Context {
public:
  using instance_extensions_t = std::vector<const char *>;
  using create_surface_fn_t =
      std::function<vk::SurfaceKHR(const vk::Instance &)>;

public:
  int width{0};
  int height{0};
  vk::Instance instance;
  vk::PhysicalDevice phy_device;
  QueueFamilyIndices que_family_indices;
  vk::SurfaceKHR surface;
  vk::Device device;

  // 队列, 理论上应该单独处理?
  vk::Queue graphic_que;
  vk::Queue present_que;

  SwapChain swapchain;

public:
  Context(const instance_extensions_t &instance_extensions,
          create_surface_fn_t fn, int w, int h)
      : width(w), height(h) {
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
    // auto graphic_que =
    //     device.getQueue(que_family_indices.graphics_family.value(), 0);
    // auto present_que =
    //     device.getQueue(que_family_indices.present_family.value(), 0);

    graphic_que =
        device.getQueue(que_family_indices.graphics_family.value(), 0);
    present_que = device.getQueue(que_family_indices.present_family.value(), 0);

    swapchain = SwapChain(phy_device, device, surface, que_family_indices,
                          /*w=*/width, /*h=*/height);
  }

  ~Context() {
    swapchain = SwapChain{};
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
    std::array<const char *, 1> layers{"VK_LAYER_KHRONOS_validation"};
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

    std::array<const char *, 1> device_extensions{"VK_KHR_swapchain"};
    create_info.setQueueCreateInfos(que_create_infos)
        .setPEnabledExtensionNames(device_extensions);
    return phy_device.createDevice(create_info);
  }

public:
  static QueueFamilyIndices
  query_family_indices(const vk::PhysicalDevice &phy_device,
                       vk::QueueFlagBits flags) {
    QueueFamilyIndices indices;
    auto queue_families = phy_device.getQueueFamilyProperties();
    for (int64_t i = 0; i < static_cast<int64_t>(queue_families.size()); i++) {
      if (queue_families[i].queueFlags & flags) {
        indices.graphics_family = static_cast<uint32_t>(i);
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
        indices.graphics_family = static_cast<uint32_t>(i);
      }
      if (phy_device.getSurfaceSupportKHR(i, surface)) {
        indices.present_family = static_cast<uint32_t>(i);
      }
      if (indices)
        break;
    }
    return indices;
  }
};

}; // namespace hf