# Shaders and ray tracing

The game draws with raylib 5 and that library's default material. `InitVoxelRenderer` in `src/voxel_renderer.c` calls `LoadMaterialDefault()` for the opaque mesh and the transparent mesh. There is no GLSL file in the repo.

Ray tracing is the last feature. Slice S30 in `SLICES.md` is blocked. A session that starts it early is doing the wrong work.

## What "shader" means here

Desktop raylib compiles GLSL 330. Shader files belong in `src/resources/shaders/` and load with `LoadShader`. The chunk mesher stays the owner of vertices. A shader samples the atlas and reads values the mesher already stored. It does not walk the voxel array.

Order:

1. Daylight and fog can start as a clear color and a fog distance in the existing renderer. That is the daylight slice, and it does not need a custom shader.
2. The chunk shader multiplies the atlas sample by the baked light level from the light slice. Opaque, cutout (leaves, glass panes later), and translucent (water, stained glass) stay three draws. Leaves already go on the transparent mesh. Cutout should not sort. Translucent should.
3. The water shader scrolls the surface so a flowing face is obvious. It uses the same fluid levels `WaterVisualHeight` already computes.
4. Ambient occlusion is a vertex value baked in the mesher, the way the game darkens corners. It is not a post pass.
5. Shadows, bloom, and a custom skybox are optional after the chunk shader. They are not parity. Do not open them as slices unless a playtest cannot read the world without them.

The chunk shader slice is done when a torch in a hole is visible, a surface at noon matches the overworld sky color `#78a7ff`, and the existing smoke test still shows a non-black world.

## Ray tracing

S30 is a second renderer, off by default. Raylib's desktop path is OpenGL 3.3. Do not depend on hardware ray-tracing extensions.

The implementation is a software DDA through the same section storage the raster world uses. One ray per pixel from the player camera, for a debug view. The center ray hits the same block the raster crosshair selects. The game boots into the raster renderer. Quitting the debug view returns to it with the world unchanged.

S30 is blocked by the light slice, the chunk shader, and the server slices. Lighting in the ray tracer, if it happens at all, reads the same light array as the raster mesh. It does not invent a third light model.

Reflections, global illumination, and a realtime path tracer are outside S30. They can be a later slice only after S30's debug view matches the crosshair.
