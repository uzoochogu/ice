# ice
A Vulkan Renderer and 3D Model viewport. Game Engine (WIP).

## Requirements
* C++ compiler supporting at least C++20 standard.
* Cmake 3.28+ (tested with this)

## Dependency management
These are the project dependencies:
1. `imgui` with glfw and vulkan bindings. Provides immediate mode GUI capabilities.
2. `glfw` - windowing (You should be able to plug in any windowing lib like SDL) - You just need to reimplement [`IceWindow`](./src/windowing.hpp) class using the library. You would also handle inputs in [`camera.cpp`](./src/camera.cpp)
3. `glm` - linear algebra, good compatibility with GLSL.
4. `stb` - loading images.
5. `tinygltf` - loading glTF scenes and models.
6. `Vulkan-Hpp` - C++ Bindings for Vulkan API. It is part of the LunarG Vulkan SDK since version 1.0.24 or part of Vulkan-Headers in `vcpkg`.

You can follow this guide for setting up dependencies:

### Setting up Vulkan
You can install Vulkan from: [`LunarG`](https://vulkan.lunarg.com/)

Note the install location. Run the `vkcube` executable in the `Bin` directory to verify a successful installation.

Copy Vulkan SDK installation path to the [`CMakeLists.txt`](./CMakeLists.txt) file.
Note that it might also be automatically installed by `vcpkg`.
### vcpkg.json
`vcpkg` is used for package and dependency management. 
A `vcpkg.json` user manifest file is provided to help install dependencies or you can install them globally.
```bash
# run this in any folder that vcpkg.json is in.
# triplet is optional, but useful on windows
path_to_vcpkg install --triplet triplet_name

# e.g
C:/dev/vcpkg/vcpkg install --triplet x64-windows
```
Global install:
```bash
path_to_vcpkg install imgui[glfw-binding,vulkan-binding]:triplet_name
path_to_vcpkg install glfw3:triplet_name
path_to_vcpkg install glm:triplet_name
path_to_vcpkg install stb:triplet_name
path_to_vcpkg integrate install
```

## Shaders compilation
For branches that make use of the shader code, you would need to compile them before use. Scripts for most platform have been provided to help in the compilation of the shaders. 
Consult [`the shaders readme`](./resources/shaders/readme.md) for more information.
You would need to do this for the [`shaders`](./resources/shaders/) directories.
```bash
# for example
cd ./resources/shaders
./compile.bat

# Linux
cd ./resources/shaders
./compile
```

## Building
Do the following steps
1. [`Install dependencies`](#dependency-management)
2. [`Compile shaders`](#shaders-compilation)
3. Generate and/or build with CMake

`CMake` is used to generate project files.
If you are working with vcpkg for dependency management, you can pass the toolchain file when you call cmake to generate:
```bash
cmake -B ./build -S ./ "-DCMAKE_TOOLCHAIN_FILE=the_path_to/vcpkg/scripts/buildsystems/vcpkg.cmake"

# e.g
cmake -B ./build -S ./ "-DCMAKE_TOOLCHAIN_FILE=C:/dev/vcpkg/scripts/buildsystems/vcpkg.cmake"
```
This command will run vcpkg install for you if you have a `vcpkg.json` file.
### Building on the command line 
After configuration and generation is done, you can run 
```bash
cd build
cmake build .
```
This will build the project.

### Using Visual Studio
On Windows, if you prefer working in Visual Studio, after generation is done, you can open the generated `sln` file and build any target you want.
The Startup project, working directory and post build commands have been automatically configured.

## Useful resources
* [`vulkan-tutorial.com`](https://vulkan-tutorial.com/)
* [`My Imgui-vulkan personal reference`](https://github.com/uzoochogu/imgui-vulkan-app)
* [`amengede's GetIntoGameDev Vulkan 2022 series`](https://github.com/amengede/getIntoGameDev/tree/main/vulkan%202022) - Created a new series ->[`Link to new series`](https://github.com/amengede/getIntoGameDev/tree/main/vulkan) 
* [`Brendan's Little Vulkan Engine`](https://github.com/blurrypiano/littleVulkanEngine/tree/main)
