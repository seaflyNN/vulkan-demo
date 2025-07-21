# vulkan-hello-world

## 构建步骤

1. 确保已安装Vulkan SDK，并配置好环境变量（如`VULKAN_SDK`）。
2. 安装CMake（建议3.10及以上）。
3. 在项目根目录下执行以下命令：

```sh
mkdir build
cd build
cmake ..
cmake --build .
```
4. 运行生成的可执行文件：

```sh
./vulkan-hello-world
```

如遇到找不到Vulkan库或头文件，请检查Vulkan SDK是否正确安装并配置。 