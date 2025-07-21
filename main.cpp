#include <vulkan/vulkan.h>

#include <iostream>
#include <vector>

int main() {

  VkApplicationInfo appInfo = {};
  VkInstanceCreateInfo instanceCreateInfo = {};
  appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
  appInfo.pApplicationName = "Hello Vulkan";
  appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
  appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);

  instanceCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
  instanceCreateInfo.pApplicationInfo = &appInfo;

  VkInstance m_stance;
  std::vector<VkPhysicalDevice> physicalDevices;

  auto result = vkCreateInstance(&instanceCreateInfo, nullptr, &m_stance);
  if (result == VK_SUCCESS) {
    uint32_t physicalDeviceCount = 0;
    result =
        vkEnumeratePhysicalDevices(m_stance, &physicalDeviceCount, nullptr);
    if (result == VK_SUCCESS) {
      physicalDevices.resize(physicalDeviceCount);
      result = vkEnumeratePhysicalDevices(m_stance, &physicalDeviceCount,
                                          physicalDevices.data());
    }
  }
  if (result != VK_SUCCESS) {
    std::cerr
        << "Failed to create Vulkan instance or enumerate physical devices."
        << std::endl;
    return -1;
  }

  // 获取物理设备信息
  VkPhysicalDeviceProperties properties;
  vkGetPhysicalDeviceProperties(physicalDevices[0], &properties);
  std::cout << "Device Name: " << properties.deviceName << std::endl;
  std::cout << "Vendor ID: " << properties.vendorID << std::endl;
  std::cout << "Device ID: " << properties.deviceID << std::endl;
  std::cout << "Device Type: " << properties.deviceType << std::endl;

  // 获取物理设备特性
  VkPhysicalDeviceFeatures out_feat;
  vkGetPhysicalDeviceFeatures(physicalDevices[0], &out_feat);
  std::cout << "Vulkan Physical Device Features:" << std::endl;
  std::cout << "Robust Buffer Access: "
            << (out_feat.robustBufferAccess ? "Enabled" : "Disabled")
            << std::endl;
  std::cout << "Full Draw Index Uint32: "
            << (out_feat.fullDrawIndexUint32 ? "Enabled" : "Disabled")
            << std::endl;
  std::cout << "Image Cube Array: "
            << (out_feat.imageCubeArray ? "Enabled" : "Disabled") << std::endl;
  std::cout << "Independent Blend: "
            << (out_feat.independentBlend ? "Enabled" : "Disabled")
            << std::endl;
  std::cout << "Geometry Shader: "
            << (out_feat.geometryShader ? "Enabled" : "Disabled") << std::endl;
  std::cout << "Tessellation Shader: "
            << (out_feat.tessellationShader ? "Enabled" : "Disabled")
            << std::endl;
  std::cout << "Sample Rate Shading: "
            << (out_feat.sampleRateShading ? "Enabled" : "Disabled")
            << std::endl;
  std::cout << "Dual Source Blend: "
            << (out_feat.dualSrcBlend ? "Enabled" : "Disabled") << std::endl;
  std::cout << "Logic Op: " << (out_feat.logicOp ? "Enabled" : "Disabled")
            << std::endl;
  std::cout << "Multi Draw Indirect: "
            << (out_feat.multiDrawIndirect ? "Enabled" : "Disabled")
            << std::endl;
  std::cout << "Draw Indirect First Instance: "
            << (out_feat.drawIndirectFirstInstance ? "Enabled" : "Disabled")
            << std::endl;
  std::cout << "Depth Clamp: " << (out_feat.depthClamp ? "Enabled" : "Disabled")
            << std::endl;
  std::cout << "Depth Bias Clamp: "
            << (out_feat.depthBiasClamp ? "Enabled" : "Disabled") << std::endl;
  std::cout << "Fill Mode Non Solid: "
            << (out_feat.fillModeNonSolid ? "Enabled" : "Disabled")
            << std::endl;
  std::cout << "Depth Bounds: "
            << (out_feat.depthBounds ? "Enabled" : "Disabled") << std::endl;
  std::cout << "Wide Lines: " << (out_feat.wideLines ? "Enabled" : "Disabled")
            << std::endl;
  std::cout << "Large Points: "
            << (out_feat.largePoints ? "Enabled" : "Disabled") << std::endl;
  std::cout << "Alpha To One: "
            << (out_feat.alphaToOne ? "Enabled" : "Disabled") << std::endl;
  std::cout << "Multi Viewport: "
            << (out_feat.multiViewport ? "Enabled" : "Disabled") << std::endl;
  std::cout << "Sampler Anisotropy: "
            << (out_feat.samplerAnisotropy ? "Enabled" : "Disabled")
            << std::endl;
  std::cout << "Texture Compression ETC2: "
            << (out_feat.textureCompressionETC2 ? "Enabled" : "Disabled")
            << std::endl;
  std::cout << "Texture Compression ASTC LDR: "
            << (out_feat.textureCompressionASTC_LDR ? "Enabled" : "Disabled")
            << std::endl;
  std::cout << "Texture Compression BC: "
            << (out_feat.textureCompressionBC ? "Enabled" : "Disabled")
            << std::endl;
  std::cout << "Occlusion Query Precise: "
            << (out_feat.occlusionQueryPrecise ? "Enabled" : "Disabled")
            << std::endl;

  // 物理设备内存
  VkPhysicalDeviceMemoryProperties mem_properties;
  vkGetPhysicalDeviceMemoryProperties(physicalDevices[0], &mem_properties);
  std::cout << "Memory Heaps: " << mem_properties.memoryHeapCount << std::endl;
  for (uint32_t i = 0; i < mem_properties.memoryHeapCount; ++i) {
    const auto &heap = mem_properties.memoryHeaps[i];
    std::cout << "Heap " << i << ": size = " << heap.size / (1024 * 1024)
              << " MB, flags = 0x" << std::hex << heap.flags << std::dec
              << std::endl;
  }

  std::cout << "Memory Types: " << mem_properties.memoryTypeCount << std::endl;
  for (uint32_t i = 0; i < mem_properties.memoryTypeCount; ++i) {
    const auto &type = mem_properties.memoryTypes[i];
    std::cout << "Type " << i << ": heapIndex = " << type.heapIndex
              << ", propertyFlags = 0x" << std::hex << type.propertyFlags
              << std::dec;
    std::cout << " [";
    if (type.propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)
      std::cout << " DEVICE_LOCAL";
    if (type.propertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)
      std::cout << " HOST_VISIBLE";
    if (type.propertyFlags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)
      std::cout << " HOST_COHERENT";
    if (type.propertyFlags & VK_MEMORY_PROPERTY_HOST_CACHED_BIT)
      std::cout << " HOST_CACHED";
    if (type.propertyFlags & VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT)
      std::cout << " LAZILY_ALLOCATED";
    std::cout << " ]" << std::endl;
  }

  // 设备队列

  // 查询并打印设备队列族信息
  uint32_t queueFamilyCount = 0;
  vkGetPhysicalDeviceQueueFamilyProperties(physicalDevices[0],
                                           &queueFamilyCount, nullptr);
  std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
  vkGetPhysicalDeviceQueueFamilyProperties(
      physicalDevices[0], &queueFamilyCount, queueFamilies.data());
  std::cout << "Queue Families: " << queueFamilyCount << std::endl;
  for (uint32_t i = 0; i < queueFamilyCount; ++i) {
    const auto &q = queueFamilies[i];
    std::cout << "Queue Family " << i << ": queueCount = " << q.queueCount
              << ", flags = 0x" << std::hex << q.queueFlags << std::dec;
    std::cout << " [";
    if (q.queueFlags & VK_QUEUE_GRAPHICS_BIT)
      std::cout << " GRAPHICS";
    if (q.queueFlags & VK_QUEUE_COMPUTE_BIT)
      std::cout << " COMPUTE";
    if (q.queueFlags & VK_QUEUE_TRANSFER_BIT)
      std::cout << " TRANSFER";
    if (q.queueFlags & VK_QUEUE_SPARSE_BINDING_BIT)
      std::cout << " SPARSE_BINDING";
    std::cout << " ]";
    std::cout << ", timestampValidBits = " << q.timestampValidBits;
    std::cout << ", minImageTransferGranularity = ("
              << q.minImageTransferGranularity.width << ", "
              << q.minImageTransferGranularity.height << ", "
              << q.minImageTransferGranularity.depth << ")" << std::endl;
  }

  // 创建逻辑设备

  // 选择支持图形的队列族
  int graphicsQueueFamily = -1;
  for (uint32_t i = 0; i < queueFamilyCount; ++i) {
    if (queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
      graphicsQueueFamily = i;
      break;
    }
  }
  if (graphicsQueueFamily == -1) {
    std::cerr << "No graphics queue family found!" << std::endl;
    return -1;
  }

  float queuePriority = 1.0f;
  VkDeviceQueueCreateInfo queueCreateInfo = {};
  queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
  queueCreateInfo.queueFamilyIndex = graphicsQueueFamily;
  queueCreateInfo.queueCount = 1;
  queueCreateInfo.pQueuePriorities = &queuePriority;

  VkDeviceCreateInfo deviceCreateInfo = {};
  deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
  deviceCreateInfo.queueCreateInfoCount = 1;
  deviceCreateInfo.pQueueCreateInfos = &queueCreateInfo;
  deviceCreateInfo.pEnabledFeatures = &out_feat; // 启用所有物理设备支持的特性

  VkDevice device;
  result = vkCreateDevice(physicalDevices[0], &deviceCreateInfo, nullptr, &device);
  if (result != VK_SUCCESS) {
    std::cerr << "Failed to create logical device!" << std::endl;
    return -1;
  }
  std::cout << "Logical device created successfully." << std::endl;

}