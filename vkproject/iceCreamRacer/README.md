# Ice Cream Car Raytracing Demo

This small executable demonstrates the existing Vulkan raytracing pipeline on a very simple scene: the `ice_cream_car` OBJ, a procedural floor, and a single light.

Build notes:

- Requires the project to be configured and built with a Vulkan SDK available.
- The demo loads `vkproject/triangleObjects/ice_cream_car/ice_cream_car.obj` directly at runtime.

Run the built `triangle_demo` executable. The window shows the car, a dark floor, and one warm light so you can inspect the raytraced shape easily.
