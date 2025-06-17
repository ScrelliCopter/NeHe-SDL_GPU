# NeHe Legacy Tutorials on SDL_GPU #

Re-implementations of the venerable [NeHe](https://nehe.gamedev.net/) legacy OpenGL
lessons with SDL3's modern [GPU API](https://wiki.libsdl.org/SDL3/CategoryGPU).
Lessons 1-10 are currently available in several different languages (C99, Rust,
and Swift)

## Lessons [01 - 05](https://nehe.gamedev.net/tutorial/lessons_01__05/22004/) ##

### Lesson 01: [Creating An ~~OpenGL~~ SDL_GPU Window](https://nehe.gamedev.net/tutorial/creating_an_opengl_window_(win32)/13001/) ###
The simplest lesson, covers creating just enough render pass to clear the
screen.

### Lesson 02: [Creating Your First Polygon & Quad](https://nehe.gamedev.net/tutorial/your_first_polygon/13002/) ###
The real polygonal "Hello world".
Instead of old-school `glBegin`/`glEnd` calls we store the shapes in static
vertex & index buffers, which are much more efficient. We also introduce our
first shader program, which simply applies our combined model-view-projection
matrix to the model in the vertex shader and shades the fragments white.

### Lesson 03: [Flat and Smooth Colors](https://nehe.gamedev.net/tutorial/adding_colour/13003/) ###
In place of `glColor3f` calls; we add RGBA components to our vertex structure
and modify our shader program to interpolate colours between vertices and
output the result to each fragment.

### Lesson 04: [Rotating A Polygon](https://nehe.gamedev.net/tutorial/rotation/14001/) ###
We have already been using a tiny matrix maths library in place of legacy GL's
`gl*` matrix stack manipulation calls, we're now using `Mtx_Rotate` in place of
`glRotatef` to apply a little animation to our previously static shapes.

### Lesson 05: [Solid Objects](https://nehe.gamedev.net/tutorial/3d_shapes/10035/) ###
Like the original, we modify our shapes to fully bring them into the 3rd
dimension, turning our triangle into a pyramid and our quad into a cube.

## Lessons [06 - 10](https://nehe.gamedev.net/tutorial/lessons_06__10/17010/) ##

### Lesson 06: [Texture Mapping](https://nehe.gamedev.net/tutorial/texture_mapping/12038/) ###
Finally, we get to texture mapping. The original lesson is split between an
older version based around `auxDIBImageLoad` from the long-deprecated
Microsoft-only GLaux library, and an updated addendum demonstrating
replacing its usage with [SOIL](https://github.com/SpartanJ/SOIL2). Because
the textures are in BMP format we can use the SDL built-in `SDL_LoadBMP`,
which lets us take advantage of the vast Surfaces API to flip the image into
the orientation the original program expects, as well as handle pixel format
conversion for us.

We modify our pipeline and shader program to replace vertex colours with a
texture sampler, we can now bind our texture to the render pass and draw a
beautiful textured cube.

### Lesson 07: [Texture Filters, Basic Lighting & Keyboard Control](https://nehe.gamedev.net/tutorial/texture_filters,_lighting_&_keyboard_control/15002/) ###
This lesson introduces multiple texture filtering styles, keyboard control for
rotating the cube, and togglable lighting.

Lighting introduces a new shader which implements just enough legacy-style
Gouraud shading to visually match the original, which required adding a few
magic constants to the lighting maths. As lighting is a separate shader
program; we create a second pipeline state for it.

OpenGL filtering & wrap modes are properties of a texture, NeHe chose to
implement switching filtering modes by uploading the same texture multiple
times and switching them at runtime. In modern graphics APIs; texture buffers
and samplers are separate constructs, so we only need to upload one static
texture and cycle samplers instead.

The last sampler enables mip-mapping. The original lesson uses GLU's
`gluBuild2DMipmaps` call (which can more or less be substituted with
[`glGenerateMipmap`](https://registry.khronos.org/OpenGL-Refpages/gl4/html/glGenerateMipmap.xhtml)
in later versions of OpenGL) to programmatically generate mip-map levels at
startup. In SDL_GPU we can use
[`SDL_GenerateMipmapsForGPUTexture`](https://wiki.libsdl.org/SDL3/SDL_GenerateMipmapsForGPUTexture)
with `layer_count_or_depth` set to $floor(\log_2\max(width,height))+1$ to
replicate OpenGL's behaviour.

### Lesson 08: [Blending](https://nehe.gamedev.net/tutorial/blending/16001/) ###
### Lesson 09: [Animated Scenes With Blended Textures](https://nehe.gamedev.net/tutorial/moving_bitmaps_in_3d_space/17001/) ###
### Lesson 10: [Loading And Moving Through A 3D World](https://nehe.gamedev.net/tutorial/loading_and_moving_through_a_3d_world/22003/) ###
