#pragma once

#include <vulkan/vulkan.hpp>

#include "utils.hpp"

//
#include <algorithm>
#include <array>
#include <cassert>
#include <functional>
#include <iostream>
#include <optional>
#include <ranges>
#include <limits>
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

struct SwapChain {
  struct SwapChainInfo {
    vk::SurfaceFormatKHR format;
    vk::Extent2D extent;
    vk::SurfaceTransformFlagBitsKHR transform;
    vk::PresentModeKHR present_mode;
    uint32_t image_count;
  };

  vk::Device device;
  vk::SwapchainKHR swapchain;

  std::vector<vk::Image> images;
  std::vector<vk::ImageView> image_views;
  SwapChainInfo swapchain_info;

public:
public:
  SwapChain() {}

  SwapChain(vk::PhysicalDevice phy_device, vk::Device device,
            vk::SurfaceKHR surface, QueueFamilyIndices indices, int w, int h)
      : device(device) {
    vk::SwapchainCreateInfoKHR create_info;

    swapchain_info = query_swapchain_support(
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
    get_images_and_image_views();
    get_views();
  }

  ~SwapChain() {
    if (!*this)
      return;

    for (auto &view : image_views)
      device.destroyImageView(view);
    device.destroySwapchainKHR(std::exchange(swapchain, VK_NULL_HANDLE));
  }
  SwapChain(const SwapChain &) = delete;
  SwapChain &operator=(const SwapChain &) = delete;
  SwapChain(SwapChain &&rhs) noexcept
      : device(std::exchange(rhs.device, {})),
        swapchain(std::exchange(rhs.swapchain, {})),
        images(std::exchange(rhs.images, {})),
        image_views(std::exchange(rhs.image_views, {})),
        swapchain_info(std::exchange(rhs.swapchain_info, {})) {}
  SwapChain &operator=(SwapChain &&rhs) noexcept {
    if (this != &rhs) {
      for (auto &view : image_views)
        device.destroyImageView(view);
      image_views.clear();

      if (*this)
        device.destroySwapchainKHR(swapchain);

      device = std::exchange(rhs.device, {});
      swapchain = std::exchange(rhs.swapchain, {});
      images = std::exchange(rhs.images, {});
      image_views = std::exchange(rhs.image_views, {});
      swapchain_info = std::exchange(rhs.swapchain_info, {});
    }
    return *this;
  }

  operator bool() const { return swapchain && device; }

  // todo: 这里应该缓存图像, 不应该每次调用都获取
  void get_images_and_image_views() {
    assert(*this);
    images = device.getSwapchainImagesKHR(swapchain);
  }

  void get_views() {
    assert(*this);
    assert(!images.empty());

    image_views.resize(images.size());
    for (int64_t i = 0; i < static_cast<int64_t>(images.size()); i++) {
      vk::ImageViewCreateInfo create_info;
      vk::ComponentMapping mapping; // rgb映射,默认都是identity, 比如rgb变成bgr?
      vk::ImageSubresourceRange
          range; // 视图访问的图像子资源范围, 比如访问mipmap的第几层,
                 // 立方体贴图的哪个面等
      range.setBaseMipLevel(0)
          .setLevelCount(1)
          .setBaseArrayLayer(0)
          .setLayerCount(1)
          .setAspectMask(vk::ImageAspectFlagBits::eColor);

      create_info.setImage(images[i])
          .setViewType(vk::ImageViewType::e2D)
          .setFormat(vk::Format::eB8G8R8A8Srgb)
          .setSubresourceRange(range);
      image_views[i] = device.createImageView(create_info);
    }
  }

public:
  static SwapChainInfo query_swapchain_support(
      const vk::PhysicalDevice &phy_device, vk::SurfaceKHR surface,
      const vk::SurfaceFormatKHR &expect_format, int w, int h) {
    auto capabilities = phy_device.getSurfaceCapabilitiesKHR(surface);
    auto present_modes = phy_device.getSurfacePresentModesKHR(surface);
    auto formats = phy_device.getSurfaceFormatsKHR(surface);

    SwapChainInfo info;
    info.format = formats[0];
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

// 垃圾shader封装
struct Shader {
  vk::Device device_;
  vk::ShaderStageFlagBits stage_{vk::ShaderStageFlagBits::eVertex};
  vk::ShaderModule module_;

public:
  Shader() {}
  Shader(vk::Device device, vk::ShaderStageFlagBits stage,
         const std::string &source)
      : device_(device), stage_(stage) {
    vk::ShaderModuleCreateInfo create_info;
    create_info.codeSize = source.size();
    create_info.pCode = reinterpret_cast<const uint32_t *>(source.data());
    module_ = device_.createShaderModule(create_info);
  }

  ~Shader() {
    if (module_)
      device_.destroyShaderModule(std::exchange(module_, VK_NULL_HANDLE));
  }

  Shader(const Shader &) = delete;
  Shader &operator=(const Shader &) = delete;
  Shader(Shader &&rhs) noexcept
      : device_(std::exchange(rhs.device_, {})),
        stage_(std::exchange(rhs.stage_, vk::ShaderStageFlagBits::eVertex)),
        module_(std::exchange(rhs.module_, {})) {}
  Shader &operator=(Shader &&rhs) noexcept {
    if (this != &rhs) {
      if (module_)
        device_.destroyShaderModule(module_);
      device_ = std::exchange(rhs.device_, {});
      stage_ = std::exchange(rhs.stage_, vk::ShaderStageFlagBits::eVertex);
      module_ = std::exchange(rhs.module_, {});
    }
    return *this;
  }
  operator bool() const { return module_; }
  auto module() const { return module_; }
  auto stage() const { return stage_; }
};

// 渲染管线
struct RenderProcess {
  // vk::GraphicsPipelineCreateInfo和vk::ComputePipelineCreateInfo两种管线
  vk::Device device_;
  vk::PipelineLayout pipeline_layout_;
  vk::RenderPass render_pass_;
  vk::Pipeline pipeline_;

public:
  RenderProcess() = default;
  ~RenderProcess() {
    if (!device_)
      return;
    if (pipeline_)
      device_.destroyPipeline(std::exchange(pipeline_, VK_NULL_HANDLE));
    if (pipeline_layout_)
      device_.destroyPipelineLayout(
          std::exchange(pipeline_layout_, VK_NULL_HANDLE));
    if (render_pass_)
      device_.destroyRenderPass(std::exchange(render_pass_, VK_NULL_HANDLE));
  }
  RenderProcess(const RenderProcess &) = delete;
  RenderProcess &operator=(const RenderProcess &) = delete;
  RenderProcess(RenderProcess &&rhs) noexcept
      : device_(std::exchange(rhs.device_, {})),
        pipeline_layout_(std::exchange(rhs.pipeline_layout_, {})),
        render_pass_(std::exchange(rhs.render_pass_, {})),
        pipeline_(std::exchange(rhs.pipeline_, {})) {}
  RenderProcess &operator=(RenderProcess &&rhs) noexcept {
    if (this != &rhs) {
      if (device_) {
        if (pipeline_)
          device_.destroyPipeline(pipeline_);
        if (pipeline_layout_)
          device_.destroyPipelineLayout(pipeline_layout_);
        if (render_pass_)
          device_.destroyRenderPass(render_pass_);
      }
      device_ = std::exchange(rhs.device_, {});
      pipeline_layout_ = std::exchange(rhs.pipeline_layout_, {});
      render_pass_ = std::exchange(rhs.render_pass_, {});
      pipeline_ = std::exchange(rhs.pipeline_, {});
    }
    return *this;
  }

  RenderProcess(vk::Device device, std::vector<Shader> shaders, int w, int h)
      : device_(device) {
    vk::GraphicsPipelineCreateInfo create_info;

    std::vector<vk::PipelineShaderStageCreateInfo> shader_stage_infos;
    shader_stage_infos.reserve(shaders.size());
    for (const auto &shader : shaders) {
      vk::PipelineShaderStageCreateInfo shader_stage_info;
      shader_stage_info.setStage(shader.stage())
          .setModule(shader.module())
          .setPName("main");
      shader_stage_infos.push_back(shader_stage_info);
    }
    create_info.setStages(shader_stage_infos);

    // 1. vertex input
    vk::PipelineVertexInputStateCreateInfo vertex_input_info;
    create_info.setPVertexInputState(&vertex_input_info);

    // 2. Vertex assembly, 顶点如何组装成图元, 比如点, 线, 三角形等, 图元重启?
    vk::PipelineInputAssemblyStateCreateInfo input_assembly_info;
    input_assembly_info.setPrimitiveRestartEnable(false).setTopology(
        vk::PrimitiveTopology::eTriangleList);
    create_info.setPInputAssemblyState(&input_assembly_info);

    // 4. viewport
    vk::PipelineViewportStateCreateInfo viewport_info;
    vk::Viewport viewport(0.0f, 0.0f, static_cast<float>(w),
                          static_cast<float>(h), 0.0f, 1.0f);
    vk::Rect2D scissor(
        vk::Offset2D{0, 0},
        vk::Extent2D{static_cast<uint32_t>(w), static_cast<uint32_t>(h)});
    viewport_info.setViewports(viewport).setScissors(scissor);
    create_info.setPViewportState(&viewport_info);

    // 5. Rasterization, 光栅化阶段, 线框模式, 背面剔除, 深度测试等
    vk::PipelineRasterizationStateCreateInfo rasterization_info;
    rasterization_info.setRasterizerDiscardEnable(false)
        .setCullMode(vk::CullModeFlagBits::eBack)
        .setFrontFace(vk::FrontFace::eClockwise)
        .setPolygonMode(vk::PolygonMode::eFill)
        .setDepthClampEnable(false)
        .setLineWidth(1.0f)
        .setDepthBiasEnable(false);
    create_info.setPRasterizationState(&rasterization_info);

    // 6. Multisampling, 多重采样, 抗锯齿
    vk::PipelineMultisampleStateCreateInfo multisample_info;
    multisample_info.setRasterizationSamples(vk::SampleCountFlagBits::e1)
        .setSampleShadingEnable(false);
    create_info.setPMultisampleState(&multisample_info);

    // 7. test - stencil, 深度测试和模板测试

    // 8. color blending, 颜色混合, alpha混合等
    vk::PipelineColorBlendStateCreateInfo blend_info;
    vk::PipelineColorBlendAttachmentState blend_attachment;
    blend_attachment.setBlendEnable(false).setColorWriteMask(
        vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
        vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA);
    blend_info.setLogicOpEnable(false).setAttachments(blend_attachment);
    create_info.setPColorBlendState(&blend_info);

    vk::PipelineLayoutCreateInfo pipeline_layout_info;
    pipeline_layout_ = device_.createPipelineLayout(pipeline_layout_info);
    create_info.setLayout(pipeline_layout_);

    // 使用swapchain的图像的layout作为color_attachment的layout
    vk::AttachmentDescription color_attachment;
    color_attachment.setFormat(vk::Format::eB8G8R8A8Srgb)
        .setSamples(vk::SampleCountFlagBits::e1)
        .setLoadOp(vk::AttachmentLoadOp::eClear)
        .setStoreOp(vk::AttachmentStoreOp::eStore)
        .setStencilLoadOp(vk::AttachmentLoadOp::eDontCare)
        .setStencilStoreOp(vk::AttachmentStoreOp::eDontCare)
        .setInitialLayout(vk::ImageLayout::eUndefined)
        .setFinalLayout(vk::ImageLayout::ePresentSrcKHR);

    vk::AttachmentReference color_attachment_ref;
    // 这里的纹理引用就是上面定义的color_attachment数组,
    // 0是color_attachment在render pass中的索引
    color_attachment_ref.setAttachment(0).setLayout(
        vk::ImageLayout::eColorAttachmentOptimal);

    vk::SubpassDescription subpass;
    subpass.setPipelineBindPoint(vk::PipelineBindPoint::eGraphics)
        .setColorAttachments(color_attachment_ref);

    vk::SubpassDependency dependency;
    dependency.setSrcSubpass(VK_SUBPASS_EXTERNAL)
        .setDstSubpass(0)
        .setSrcStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput)
        .setDstStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput)
        .setDstAccessMask(vk::AccessFlagBits::eColorAttachmentWrite);

    vk::RenderPassCreateInfo render_pass_info;
    render_pass_info.setAttachments(color_attachment)
        .setSubpasses(subpass)
        .setDependencies(dependency);
    render_pass_ = device_.createRenderPass(render_pass_info);

    create_info.setRenderPass(render_pass_).setSubpass(0);

    pipeline_ = device_.createGraphicsPipeline({}, create_info).value;
  }
};

// 命令
struct Render {
  vk::Device device;
  QueueFamilyIndices indices;
  vk::CommandPool command_pool;
  vk::CommandBuffer command_buffer;

public:
  Render() {}
  Render(vk::Device device, QueueFamilyIndices indices)
      : device(device), indices(indices) {
    init_command_pool(indices);
    alloc_command_buffer();
  }
  ~Render() {
    if (command_pool) {
      if (command_buffer)
        device.freeCommandBuffers(command_pool,
                                  std::exchange(command_buffer, {}));
      device.destroyCommandPool(std::exchange(command_pool, VK_NULL_HANDLE));
    }
  }

  Render(const Render &) = delete;
  Render &operator=(const Render &) = delete;

  Render(Render &&rhs) noexcept
      : device(std::exchange(rhs.device, {})),
        indices(std::exchange(rhs.indices, {})),
        command_pool(std::exchange(rhs.command_pool, {})),
        command_buffer(std::exchange(rhs.command_buffer, {})) {}

  Render &operator=(Render &&rhs) noexcept {
    if (this != &rhs) {
      if (command_pool) {
        if (command_buffer)
          device.freeCommandBuffers(command_pool,
                                    std::exchange(command_buffer, {}));
        device.destroyCommandPool(std::exchange(command_pool, VK_NULL_HANDLE));
      }

      device = std::exchange(rhs.device, {});
      indices = std::exchange(rhs.indices, {});
      command_pool = std::exchange(rhs.command_pool, {});
      command_buffer = std::exchange(rhs.command_buffer, {});
    }
    return *this;
  }

private:
  void init_command_pool(const QueueFamilyIndices &indices) {
    vk::CommandPoolCreateInfo create_info;
    create_info.setQueueFamilyIndex(indices.graphics_family.value())
        .setFlags(vk::CommandPoolCreateFlagBits::eResetCommandBuffer);
    command_pool = device.createCommandPool(create_info);
  }
  void alloc_command_buffer() {
    vk::CommandBufferAllocateInfo alloc_info;
    alloc_info.setCommandPool(command_pool)
        .setLevel(vk::CommandBufferLevel::ePrimary)
        .setCommandBufferCount(1);
    command_buffer = device.allocateCommandBuffers(alloc_info)[0];
  }
};

class Context {
public:
  using instance_extensions_t = std::vector<const char *>;
  using create_surface_fn_t =
      std::function<vk::SurfaceKHR(const vk::Instance &)>;

private:
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

  // Shader vert_shader;
  // Shader frag_shader;

  std::string vertex_shader_path;
  std::string fragment_shader_path;

  RenderProcess render_process;

  std::vector<vk::Framebuffer> framebuffers;

  Render renderer;

  vk::Semaphore image_available_semaphore;
  vk::Semaphore render_finished_semaphore;
  vk::Fence in_flight_fence;

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

    init_sync_objects();
  }

  ~Context() {
    if (device) {
      device.waitIdle();
    }

    for (auto &framebuffer : framebuffers)
      device.destroyFramebuffer(framebuffer);

    if (in_flight_fence)
      device.destroyFence(std::exchange(in_flight_fence, VK_NULL_HANDLE));
    if (image_available_semaphore)
      device.destroySemaphore(
          std::exchange(image_available_semaphore, VK_NULL_HANDLE));
    if (render_finished_semaphore)
      device.destroySemaphore(
          std::exchange(render_finished_semaphore, VK_NULL_HANDLE));

    renderer = Render{};
    render_process = RenderProcess{};
    swapchain = SwapChain{};
    // todo: 把device之类的也raii化才行
    device.destroy();
    if (surface != VK_NULL_HANDLE) {
      instance.destroySurfaceKHR(surface);
      surface = VK_NULL_HANDLE;
    }
    instance.destroy();
  }

  Context(const Context &) = delete;
  Context &operator=(const Context &) = delete;

  void init_shader(const std::string &vertex_path,
                   const std::string &fragment_path) {
    // auto vertex_source = helper::read_file(vertex_path);
    // auto fragment_source = helper::read_file(fragment_path);
    // vert_shader =
    //     Shader(device, vk::ShaderStageFlagBits::eVertex, vertex_source);
    // frag_shader =
    //     Shader(device, vk::ShaderStageFlagBits::eFragment, fragment_source);
    vertex_shader_path = vertex_path;
    fragment_shader_path = fragment_path;
  }

  void init_render_process() {
    std::vector<Shader> shaders;
    shaders.emplace_back(device, vk::ShaderStageFlagBits::eVertex,
                         helper::read_file(vertex_shader_path));
    shaders.emplace_back(device, vk::ShaderStageFlagBits::eFragment,
                         helper::read_file(fragment_shader_path));
    render_process = RenderProcess(device, std::move(shaders), width, height);
  }

  void create_framebuffers() {
    framebuffers.resize(swapchain.images.size());
    for (int64_t i = 0; i < static_cast<int64_t>(swapchain.images.size());
         i++) {
      vk::ImageView attachments[] = {swapchain.image_views[i]};
      vk::FramebufferCreateInfo create_info;
      create_info.setRenderPass(render_process.render_pass_)
          .setAttachments(attachments)
          .setWidth(width)
          .setHeight(height)
          .setLayers(1);
      framebuffers[i] = device.createFramebuffer(create_info);
    }
  }

  void init_render() {
    //
    renderer = Render(device, que_family_indices);
  }

  void render() {
    auto wait_result = device.waitForFences(in_flight_fence, true,
                                            std::numeric_limits<uint64_t>::max());
    assert(wait_result == vk::Result::eSuccess);
    device.resetFences(in_flight_fence);

    // 从交换链获取当前可用的图像索引
    auto result = device.acquireNextImageKHR(
      swapchain.swapchain, std::numeric_limits<uint64_t>::max(),
      image_available_semaphore, {});
    assert(result.result == vk::Result::eSuccess);
    auto image_index = result.value;
    auto &cmd_buf = renderer.command_buffer;
    cmd_buf.reset();
    cmd_buf.begin({vk::CommandBufferUsageFlagBits::eOneTimeSubmit});
    {
      cmd_buf.bindPipeline(vk::PipelineBindPoint::eGraphics,
                           render_process.pipeline_);
      vk::RenderPassBeginInfo render_pass_info;
      vk::Rect2D area{vk::Offset2D{0, 0}, swapchain.swapchain_info.extent};
      vk::ClearValue clear_color(std::array<float, 4>{0.0f, 0.0f, 0.0f, 1.0f});
      render_pass_info.setRenderPass(render_process.render_pass_)
          .setFramebuffer(framebuffers[image_index])
          .setClearValues(clear_color)
          .setRenderArea(area);
      cmd_buf.beginRenderPass(render_pass_info, vk::SubpassContents::eInline);
      {
        cmd_buf.bindPipeline(vk::PipelineBindPoint::eGraphics,
                             render_process.pipeline_);
        cmd_buf.draw(3, 1, 0, 0);
      }
      cmd_buf.endRenderPass();
    }
    cmd_buf.end();

    vk::PipelineStageFlags wait_stage_mask =
        vk::PipelineStageFlagBits::eColorAttachmentOutput;
    vk::SubmitInfo submit_info;
    submit_info.setWaitSemaphores(image_available_semaphore)
        .setWaitDstStageMask(wait_stage_mask)
        .setCommandBuffers(cmd_buf)
        .setSignalSemaphores(render_finished_semaphore);
    graphic_que.submit(submit_info, in_flight_fence);

    vk::PresentInfoKHR present_info;
    present_info.setWaitSemaphores(render_finished_semaphore)
        .setSwapchains(swapchain.swapchain)
        .setImageIndices(image_index);
    auto present_result = present_que.presentKHR(present_info);
    assert(present_result == vk::Result::eSuccess ||
           present_result == vk::Result::eSuboptimalKHR);
    // device.waitForFences(in_flight_fence, true, std::numeric_limits<uint64_t>::max());
    present_que.waitIdle();
  }

private:
  void init_sync_objects() {
    vk::SemaphoreCreateInfo semaphore_info;
    image_available_semaphore = device.createSemaphore(semaphore_info);
    render_finished_semaphore = device.createSemaphore(semaphore_info);

    vk::FenceCreateInfo fence_info;
    fence_info.setFlags(vk::FenceCreateFlagBits::eSignaled);
    in_flight_fence = device.createFence(fence_info);
  }

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

    std::array<const char *, 1> device_extensions{
        VK_KHR_SWAPCHAIN_EXTENSION_NAME};
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