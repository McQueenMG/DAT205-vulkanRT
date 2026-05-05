# Triangle Demo

This small executable demonstrates rendering a single triangle using the existing `vulkan_renderer` infrastructure.

Build notes:
- Requires the project to be configured and built with a Vulkan SDK available.
- If `glslangValidator` is installed, CMake will compile GLSL into SPIR-V at build time.
- Otherwise compile shaders manually:
  ```bash
  glslangValidator -V triangle.vert -o triangle.vert.spv
  glslangValidator -V triangle.frag -o triangle.frag.spv
  ```

Run: copy the generated `triangle.vert.spv` and `triangle.frag.spv` to the same folder as the built executable (CMake will do this automatically if `glslangValidator` is found).
