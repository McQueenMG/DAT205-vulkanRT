glslc --target-spv=spv1.5 raytrace.rgen -o ../../../shaders/raytrace.rgen.bin
glslc --target-spv=spv1.5 raytrace.rchit -o ../../../shaders/raytrace.rchit.bin
glslc --target-spv=spv1.5 raytrace.rmiss -o ../../../shaders/raytrace.rmiss.bin
glslc --target-spv=spv1.5 raytrace.rahit -o ../../../shaders/raytrace.rahit.bin
glslc --target-spv=spv1.5 standard.vert -o ../../../shaders/standard.vert.bin
glslc --target-spv=spv1.5 standard.frag -o ../../../shaders/standard.frag.bin
glslc --target-spv=spv1.5 blit.vert -o ../../../shaders/blit.vert.bin
glslc --target-spv=spv1.5 blit.frag -o ../../../shaders/blit.frag.bin
glslc --target-spv=spv1.5 gbuffer.vert -o ../../../shaders/gbuffer.vert.bin
glslc --target-spv=spv1.5 gbuffer.geom -o ../../../shaders/gbuffer.geom.bin
glslc --target-spv=spv1.5 gbuffer.frag -o ../../../shaders/gbuffer.frag.bin


#Command Line: glslc --target-spv=spv1.5 %(FullPath)  -o $(SolutionDir)../shaders/%(Filename)%(Extension).bin
#Description: Compiling Shader %(Filename)%(Extension) to $(SolutionDir)../shaders/%(Filename)%(Extension).bin
#Outputs: $(SolutionDir)../shaders/%(Filename)%(Extension).bin
