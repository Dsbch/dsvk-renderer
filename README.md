3D renderer written from scratch with C++20 and Vulkan 1.3.
Task shader pipeline is used. For now I only implemented a working, somewhat scalable opaque pass and an accumulation pass with OIT. The goal is to render a big open-world scene with a lot of topology. Because the task shader pipeline is used, I have three types of culling: frustum, occlusion and backface (frustum + occlusion in the compute shader, and backface in the task + mesh shader). The renderer supports simple skeletal animations (for now it just loops each animation).

See the [roadmap](#roadmap).

![Meshlet culling](docs/culling.gif)
![blending](docs/oit.png)

## Features

- **Task shader pipeline.** Instanced rendering with LOD level selection in the compute shader.
- **Two-phase HZB occlusion culling** runs in compute. I need to implement a GPU prefix-sum algorithm to make the rendering truly GPU-driven.
- **Order-independent transparency**.
- **PBR**, metallic-roughness material workflow.
- **Skeletal animation** with per-meshlet culling for animated geometry (has bugs, see [roadmap](#roadmap)).
- **ECS architecture.** The scene is built on an ECS (EnTT).
- **In-app GPU profiler**, query-based profiling window.

## Stack

- **Language / API:** C++20, Vulkan, HLSL shaders
- **Vulkan setup:** [vk-bootstrap](https://github.com/charles-lunarg/vk-bootstrap), VMA
- **Assets:** [cgltf](https://github.com/jkuhlmann/cgltf), [meshoptimizer](https://github.com/zeux/meshoptimizer), [basis_universal](https://github.com/BinomialLLC/basis_universal), stb_image
- **Core libs:** [EnTT](https://github.com/skypjack/entt), [GLFW](https://github.com/glfw/glfw), [GLM](https://github.com/g-truc/glm), [Dear ImGui](https://github.com/ocornut/imgui), [coost](https://github.com/idealvin/coost), [spdlog](https://github.com/gabime/spdlog), [nlohmann/json](https://github.com/nlohmann/json)

## Building

Windows / x64 only for now.

1. Install the [Vulkan SDK](https://vulkan.lunarg.com/sdk/home).
2. Generate the Visual Studio 2026 solution with [premake5](https://premake.github.io/):

   ```
    premake5 --mode=sandbox vs2026
   ```

3. Open the generated solution and build in `Release`.

## Controls

Run a **Release** build of the sandbox:

| Key | Action |
| --- | --- |
| `WASD`  | Controls |
| `M`  | Toggle cursor |
| `B` | Toggle debug camera to test culling |

## Roadmap

**In progress**
- **Fully GPU-driven indirect rendering.** GPU prefix-sum algorithm for the command buffer after culling and LOD level selection. This will boost performance because the amplification rate becomes higher.
- **Per-frame-in-flight resource buffering.** Need to separate joint matrices/per-instance/draw buffers for each frame in flight.
- **Separate main thread into render thread and game thread.** This will solve a lot of problems and bugs.

**Planned**
- Hard and soft shadows.
- Global illumination and reflections with radiance cascades.
- Post-processing.
- CPU-side Jolt physics.
- Optional: custom scene format with full ECS serialization, and gizmos.

**Known bugs**
- Low amplification rate, need to implement GPU prefix-sum after compute culling.
- Flickering of meshlets because I have 3 frames in flight and they rewrite CMD buffers, need to implement per-frame-in-flight resource buffering.
