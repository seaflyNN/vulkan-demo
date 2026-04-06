#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include "context.hpp"

#include "SDL3/SDL.h"
#include "SDL3/SDL_vulkan.h"

int main() {
  if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS)) {
    return -1;
  }

  int width = 800;
  int height = 600;
  uint64_t window_flag = SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE |
                         SDL_WINDOW_HIDDEN | SDL_WINDOW_HIGH_PIXEL_DENSITY;
  auto window = SDL_CreateWindow("Vulkan", width, height, window_flag);
  if (!window) {
    SDL_Quit();
    return -1;
  }

  // 拿到vulkan扩展列表
  unsigned int count{0};
  auto exts = SDL_Vulkan_GetInstanceExtensions(&count);
  if (!exts) {
    std::cerr << "SDL_Vulkan_GetInstanceExtensions failed: " << SDL_GetError()
              << std::endl;
    SDL_DestroyWindow(window);
    SDL_Quit();
    return -1;
  }
  std::vector<const char *> extensions(exts, exts + count);
  for (auto &&ext : extensions) {
    std::cout << ext << std::endl;
  }

  //
  auto ctx = std::make_unique<hf::Context>(
      extensions,
      [&](vk::Instance instance) -> vk::SurfaceKHR {
        VkSurfaceKHR surface = VK_NULL_HANDLE;
        if (!SDL_Vulkan_CreateSurface(window, static_cast<VkInstance>(instance),
                                      nullptr, &surface)) {
          std::cerr << "SDL_Vulkan_CreateSurface failed: " << SDL_GetError()
                    << std::endl;
          return VK_NULL_HANDLE;
        }
        return vk::SurfaceKHR(surface);
      },
      width, height);

#ifdef HF_SHADER_DIR
  const std::string shader_dir = HF_SHADER_DIR;
#else
  const std::string shader_dir = "shader";
#endif

  ctx->init_shader(shader_dir + "/vert.spv", shader_dir + "/frag.spv");
  ctx->init_render_process();
  ctx->create_framebuffers();
  ctx->init_render();
  // 画三角形
  
  SDL_ShowWindow(window);

  bool running = true;
  while (running) {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
      if (event.type == SDL_EVENT_QUIT) {
        running = false;
      }
    }

    ctx->render();
  }

  ctx.reset();
  if (window) {
    SDL_DestroyWindow(window);
  }
  SDL_Quit();
  return 0;
}
