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

This is also the point we need to add depth testing. In OpenGL this simply
requires enabling the `GL_DEPTH_TEST` capability and passing the
`GL_DEPTH_BUFFER_BIT` flag to `glClear`, however, modern APIs require depth
buffer(s) to be manually created and managed by the client application.
SDL_GPU thankfully handles resource cycling for us but depth texture management
boilerplate is long & repetitive enough to justify being factored out into the
utility library.

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
This lesson demonstrates use of additive blending on the previous lesson's
cube.

The original lesson simply toggles the `GL_BLEND` capability to enable and
disable blending. Since we are re-using the lighting setup; we'll need four
pipelines: the existing lit & unlit shader programs, and lit & unlit pipelines
with depth testing disable and blending enabled; to cover all possible toggle
states.

The original uses `glColor4f(1, 1, 1, 0.5)` to halve the opacity when blending
is toggled on, so we extend Lesson 06's basic textured shader to add a colour
tinting uniform. Note that OpenGL does not apply per-vertex colour to lit faces
without `GL_COLOR_MATERIAL` explicitly enabled, and the original takes
advantage of this to only reduce opacity in the unlit blended case; ergo we
don't need a tint uniform in the light shader.

### Lesson 09: [Animated Scenes With Blended Textures](https://nehe.gamedev.net/tutorial/moving_bitmaps_in_3d_space/17001/) ###
This lesson draws 50 (or 100 in "twinkle" mode) star particles with random
colours; animated in a pretty concentric spiral.

The current implementation instances the sprites using a storage buffer that
contains each star's model-view matrix and colour, as all 100 stars are drawn
in a single draw call. The approach used is still far from optimal and is also
currently bugged on D3D12, so I may end up using a different approach later.

### Lesson 10: [Loading And Moving Through A 3D World](https://nehe.gamedev.net/tutorial/loading_and_moving_through_a_3d_world/22003/) ###
Here it is, the ever infamous 3D world tutorial. Gaze in wonderment at the
majesty of Mud.bmp as you explore the caverns of presumably the author's face
processed with dazzling late 90's Photoshop effects, forever embossed in what
looks to be some kind of granite texture. Step into the driver's seat with your
computer keyboard as you explore this truly interactive digital Mount Rushmore
through immersive tank controls.

This lesson loads the 3D environment from a text file as a triangle list, the
loader logic is almost identical with the only significant difference being the
usage of a vertex buffer instead of looping over each triangle within
`glBegin` & `glEnd`.

The togglable blending is carried over from the last lesson and lighting is
dropped, so only two pipeline states are required. 
