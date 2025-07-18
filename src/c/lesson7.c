/*
 * SPDX-FileCopyrightText: (C) 2025 a dinosaur
 * SPDX-License-Identifier: Zlib
 */

#include "nehe.h"
#include <float.h>


typedef struct
{
	float x, y, z;
	float nx, ny, nz;
	float u, v;
} Vertex;

static const Vertex vertices[] =
{
	// Front Face
	{ -1.0f, -1.0f,  1.0f,   0.0f,  0.0f,  1.0f,  0.0f, 0.0f },
	{  1.0f, -1.0f,  1.0f,   0.0f,  0.0f,  1.0f,  1.0f, 0.0f },
	{  1.0f,  1.0f,  1.0f,   0.0f,  0.0f,  1.0f,  1.0f, 1.0f },
	{ -1.0f,  1.0f,  1.0f,   0.0f,  0.0f,  1.0f,  0.0f, 1.0f },
	// Back Face
	{ -1.0f, -1.0f, -1.0f,   0.0f,  0.0f, -1.0f,  1.0f, 0.0f },
	{ -1.0f,  1.0f, -1.0f,   0.0f,  0.0f, -1.0f,  1.0f, 1.0f },
	{  1.0f,  1.0f, -1.0f,   0.0f,  0.0f, -1.0f,  0.0f, 1.0f },
	{  1.0f, -1.0f, -1.0f,   0.0f,  0.0f, -1.0f,  0.0f, 0.0f },
	// Top Face
	{ -1.0f,  1.0f, -1.0f,   0.0f,  1.0f,  0.0f,  0.0f, 1.0f },
	{ -1.0f,  1.0f,  1.0f,   0.0f,  1.0f,  0.0f,  0.0f, 0.0f },
	{  1.0f,  1.0f,  1.0f,   0.0f,  1.0f,  0.0f,  1.0f, 0.0f },
	{  1.0f,  1.0f, -1.0f,   0.0f,  1.0f,  0.0f,  1.0f, 1.0f },
	// Bottom Face
	{ -1.0f, -1.0f, -1.0f,   0.0f, -1.0f,  0.0f,  1.0f, 1.0f },
	{  1.0f, -1.0f, -1.0f,   0.0f, -1.0f,  0.0f,  0.0f, 1.0f },
	{  1.0f, -1.0f,  1.0f,   0.0f, -1.0f,  0.0f,  0.0f, 0.0f },
	{ -1.0f, -1.0f,  1.0f,   0.0f, -1.0f,  0.0f,  1.0f, 0.0f },
	// Right face
	{  1.0f, -1.0f, -1.0f,   1.0f,  0.0f,  0.0f,  1.0f, 0.0f },
	{  1.0f,  1.0f, -1.0f,   1.0f,  0.0f,  0.0f,  1.0f, 1.0f },
	{  1.0f,  1.0f,  1.0f,   1.0f,  0.0f,  0.0f,  0.0f, 1.0f },
	{  1.0f, -1.0f,  1.0f,   1.0f,  0.0f,  0.0f,  0.0f, 0.0f },
	// Left Face
	{ -1.0f, -1.0f, -1.0f,  -1.0f,  0.0f,  0.0f,  0.0f, 0.0f },
	{ -1.0f, -1.0f,  1.0f,  -1.0f,  0.0f,  0.0f,  1.0f, 0.0f },
	{ -1.0f,  1.0f,  1.0f,  -1.0f,  0.0f,  0.0f,  1.0f, 1.0f },
	{ -1.0f,  1.0f, -1.0f,  -1.0f,  0.0f,  0.0f,  0.0f, 1.0f }
};

static const uint16_t indices[] =
{
	 0,  1,  2,   2,  3,  0,  // Front
	 4,  5,  6,   6,  7,  4,  // Back
	 8,  9, 10,  10, 11,  8,  // Top
	12, 13, 14,  14, 15, 12,  // Bottom
	16, 17, 18,  18, 19, 16,  // Right
	20, 21, 22,  22, 23, 20   // Left
};


static SDL_GPUGraphicsPipeline* psoUnlit = NULL, * psoLight = NULL;
static SDL_GPUBuffer* vtxBuffer = NULL;
static SDL_GPUBuffer* idxBuffer = NULL;
static SDL_GPUSampler* samplers[3] = { NULL, NULL, NULL };
static SDL_GPUTexture* texture = NULL;

static Mtx projection;

static bool lighting = false;
static struct Light { float ambient[4], diffuse[4], position[4]; } light =
{
	.ambient  = { 0.5f, 0.5f, 0.5f, 1.0f },
	.diffuse  = { 1.0f, 1.0f, 1.0f, 1.0f },
	.position = { 0.0f, 0.0f, 2.0f, 1.0f }
};

static int filter = 0;

static float xRot = 0.0f, yRot = 0.0f;
static float xSpeed = 0.0f, ySpeed = 0.0f;
static float z = -5.0f;


static bool Lesson7_Init(NeHeContext* restrict ctx)
{
	SDL_GPUShader* vertexShaderUnlit, * fragmentShaderUnlit;
	SDL_GPUShader* vertexShaderLight, * fragmentShaderLight;
	if (!NeHe_LoadShaders(ctx, &vertexShaderUnlit, &fragmentShaderUnlit, "lesson6",
		&(const NeHeShaderProgramCreateInfo){ .vertexUniforms = 1, .fragmentSamplers = 1 }))
	{
		return false;
	}
	if (!NeHe_LoadShaders(ctx, &vertexShaderLight, &fragmentShaderLight, "lesson7",
		&(const NeHeShaderProgramCreateInfo){ .vertexUniforms = 2, .fragmentSamplers = 1 }))
	{
		SDL_ReleaseGPUShader(ctx->device, fragmentShaderUnlit);
		SDL_ReleaseGPUShader(ctx->device, vertexShaderUnlit);
		return false;
	}

	const SDL_GPUVertexAttribute vertexAttribs[] =
	{
		{
			.location = 0,
			.buffer_slot = 0,
			.format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3,
			.offset = offsetof(Vertex, x)
		},
		{
			.location = 1,
			.buffer_slot = 0,
			.format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2,
			.offset = offsetof(Vertex, u)
		},
		{
			.location = 2,
			.buffer_slot = 0,
			.format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3,
			.offset = offsetof(Vertex, nx)
		}
	};

	const SDL_GPUVertexInputState vertexInput =
	{
		.vertex_buffer_descriptions = &(const SDL_GPUVertexBufferDescription)
		{
			.slot = 0,
			.pitch = sizeof(Vertex),
			.input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX
		},
		.num_vertex_buffers = 1,
		.vertex_attributes = vertexAttribs,
		.num_vertex_attributes = SDL_arraysize(vertexAttribs)
	};

	const SDL_GPURasterizerState rasterizer =
	{
		.fill_mode = SDL_GPU_FILLMODE_FILL,
		.cull_mode = SDL_GPU_CULLMODE_NONE,
		.front_face = SDL_GPU_FRONTFACE_COUNTER_CLOCKWISE,  // Right-handed coordinates
		.enable_depth_clip = true  // OpenGL-like clip behaviour
	};

	const SDL_GPUDepthStencilState depthStencil =
	{
		.compare_op = SDL_GPU_COMPAREOP_LESS_OR_EQUAL,
		.enable_depth_test = true,
		.enable_depth_write = true
	};

	const SDL_GPUGraphicsPipelineTargetInfo targetInfo =
	{
		.color_target_descriptions = &(const SDL_GPUColorTargetDescription)
		{
			.format = SDL_GetGPUSwapchainTextureFormat(ctx->device, ctx->window)
		},
		.num_color_targets = 1,
		.depth_stencil_format = appConfig.createDepthFormat,
		.has_depth_stencil_target = true
	};

	psoUnlit = SDL_CreateGPUGraphicsPipeline(ctx->device, &(const SDL_GPUGraphicsPipelineCreateInfo)
	{
		.vertex_shader = vertexShaderUnlit,
		.fragment_shader = fragmentShaderUnlit,
		.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST,
		.vertex_input_state = vertexInput,
		.rasterizer_state = rasterizer,
		.depth_stencil_state = depthStencil,
		.target_info = targetInfo
	});
	SDL_ReleaseGPUShader(ctx->device, fragmentShaderUnlit);
	SDL_ReleaseGPUShader(ctx->device, vertexShaderUnlit);
	if (!psoUnlit)
	{
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "SDL_CreateGPUGraphicsPipeline: %s", SDL_GetError());
		SDL_ReleaseGPUShader(ctx->device, fragmentShaderLight);
		SDL_ReleaseGPUShader(ctx->device, vertexShaderLight);
		return false;
	}

	psoLight = SDL_CreateGPUGraphicsPipeline(ctx->device, &(const SDL_GPUGraphicsPipelineCreateInfo)
	{
		.vertex_shader = vertexShaderLight,
		.fragment_shader = fragmentShaderLight,
		.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST,
		.vertex_input_state = vertexInput,
		.rasterizer_state = rasterizer,
		.depth_stencil_state = depthStencil,
		.target_info = targetInfo
	});
	SDL_ReleaseGPUShader(ctx->device, fragmentShaderLight);
	SDL_ReleaseGPUShader(ctx->device, vertexShaderLight);
	if (!psoLight)
	{
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "SDL_CreateGPUGraphicsPipeline: %s", SDL_GetError());
		return false;
	}

	if ((texture = NeHe_LoadTexture(ctx, "Data/Crate.bmp", true, true)) == NULL)
	{
		return false;
	}

	samplers[0] = SDL_CreateGPUSampler(ctx->device, &(const SDL_GPUSamplerCreateInfo)
	{
		.min_filter = SDL_GPU_FILTER_NEAREST,
		.mag_filter = SDL_GPU_FILTER_NEAREST
	});
	samplers[1] = SDL_CreateGPUSampler(ctx->device, &(const SDL_GPUSamplerCreateInfo)
	{
		.min_filter = SDL_GPU_FILTER_LINEAR,
		.mag_filter = SDL_GPU_FILTER_LINEAR
	});
	samplers[2] = SDL_CreateGPUSampler(ctx->device, &(const SDL_GPUSamplerCreateInfo)
	{
		.min_filter = SDL_GPU_FILTER_LINEAR,
		.mag_filter = SDL_GPU_FILTER_LINEAR,
		.mipmap_mode = SDL_GPU_SAMPLERMIPMAPMODE_NEAREST,
		.max_lod = FLT_MAX
	});
	if (!samplers[0] || !samplers[1] || !samplers[2])
	{
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "SDL_CreateGPUSampler: %s", SDL_GetError());
		return false;
	}

	if (!NeHe_CreateVertexIndexBuffer(ctx, &vtxBuffer, &idxBuffer,
		vertices, sizeof(vertices),
		indices, sizeof(indices)))
	{
		return false;
	}

	return true;
}

static void Lesson7_Quit(NeHeContext* restrict ctx)
{
	SDL_ReleaseGPUBuffer(ctx->device, idxBuffer);
	SDL_ReleaseGPUBuffer(ctx->device, vtxBuffer);
	for (int i = SDL_arraysize(samplers) - 1; i > 0; --i)
	{
		SDL_ReleaseGPUSampler(ctx->device, samplers[i]);
	}
	SDL_ReleaseGPUTexture(ctx->device, texture);
	SDL_ReleaseGPUGraphicsPipeline(ctx->device, psoLight);
	SDL_ReleaseGPUGraphicsPipeline(ctx->device, psoUnlit);
}

static void Lesson7_Resize(NeHeContext* restrict ctx, int width, int height)
{
	(void)ctx;

	// Avoid division by zero by clamping height
	height = SDL_max(height, 1);
	// Recalculate projection matrix
	projection = Mtx_Perspective(45.0f, (float)width / (float)height, 0.1f, 100.0f);
}

static void Lesson7_Draw(NeHeContext* restrict ctx, SDL_GPUCommandBuffer* restrict cmd,
	SDL_GPUTexture* restrict swapchain, unsigned swapchainW, unsigned swapchainH)
{
	(void)swapchainW; (void)swapchainH;

	const SDL_GPUColorTargetInfo colorInfo =
	{
		.texture = swapchain,
		.clear_color = { 0.0f, 0.0f, 0.0f, 0.5f },
		.load_op = SDL_GPU_LOADOP_CLEAR,
		.store_op = SDL_GPU_STOREOP_STORE
	};

	const SDL_GPUDepthStencilTargetInfo depthInfo =
	{
		.texture = ctx->depthTexture,
		.clear_depth = 1.0f,  // Ensure depth buffer clears to furthest value
		.load_op = SDL_GPU_LOADOP_CLEAR,
		.store_op = SDL_GPU_STOREOP_DONT_CARE,
		.stencil_load_op = SDL_GPU_LOADOP_DONT_CARE,
		.stencil_store_op = SDL_GPU_STOREOP_DONT_CARE,
		.cycle = true
	};

	// Begin pass & bind pipeline state
	SDL_GPURenderPass* pass = SDL_BeginGPURenderPass(cmd, &colorInfo, 1, &depthInfo);
	SDL_BindGPUGraphicsPipeline(pass, lighting ? psoLight : psoUnlit);

	// Bind texture
	SDL_BindGPUFragmentSamplers(pass, 0, &(const SDL_GPUTextureSamplerBinding)
	{
		.texture = texture,
		.sampler = samplers[filter]
	}, 1);

	// Bind vertex & index buffers
	SDL_BindGPUVertexBuffers(pass, 0, &(const SDL_GPUBufferBinding)
	{
		.buffer = vtxBuffer,
		.offset = 0
	}, 1);
	SDL_BindGPUIndexBuffer(pass, &(const SDL_GPUBufferBinding)
	{
		.buffer = idxBuffer,
		.offset = 0
	}, SDL_GPU_INDEXELEMENTSIZE_16BIT);

	// Setup the cube's model matrix
	Mtx model = Mtx_Translation(0.0f, 0.0f, z);
	Mtx_Rotate(&model, xRot, 1.0f, 0.0f, 0.0f);
	Mtx_Rotate(&model, yRot, 0.0f, 1.0f, 0.0f);

	// Push shader uniforms
	if (lighting)
	{
		struct { Mtx model, projection; } u = { model, projection };
		SDL_PushGPUVertexUniformData(cmd, 0, &u, sizeof(u));
		SDL_PushGPUVertexUniformData(cmd, 1, &light, sizeof(light));
	}
	else
	{
		Mtx modelViewProj = Mtx_Multiply(&projection, &model);
		SDL_PushGPUVertexUniformData(cmd, 0, &modelViewProj, sizeof(Mtx));
	}

	// Draw textured cube
	SDL_DrawGPUIndexedPrimitives(pass, SDL_arraysize(indices), 1, 0, 0, 0);

	SDL_EndGPURenderPass(pass);

	const bool* keys = SDL_GetKeyboardState(NULL);

	if (keys[SDL_SCANCODE_PAGEUP])   { z -= 0.02f; }
	if (keys[SDL_SCANCODE_PAGEDOWN]) { z += 0.02f; }
	if (keys[SDL_SCANCODE_UP])   { xSpeed -= 0.01f; }
	if (keys[SDL_SCANCODE_DOWN]) { xSpeed += 0.01f; }
	if (keys[SDL_SCANCODE_RIGHT]) { ySpeed += 0.1f; }
	if (keys[SDL_SCANCODE_LEFT])  { ySpeed -= 0.1f; }

	xRot += xSpeed;
	yRot += ySpeed;
}

static void Lesson7_Key(NeHeContext* ctx, SDL_Keycode key, bool down, bool repeat)
{
	(void)ctx;

	if (down && !repeat)
	{
		switch (key)
		{
		case SDLK_L:
			lighting = !lighting;
			break;
		case SDLK_F:
			filter = (filter + 1) % (int)SDL_arraysize(samplers);
			break;
		default:
			break;
		}
	}
}


const struct AppConfig appConfig =
{
	.title = "NeHe's Textures, Lighting & Keyboard Tutorial",
	.width = 640, .height = 480,
	.createDepthFormat = SDL_GPU_TEXTUREFORMAT_D16_UNORM,
	.init = Lesson7_Init,
	.quit = Lesson7_Quit,
	.resize = Lesson7_Resize,
	.draw = Lesson7_Draw,
	.key = Lesson7_Key
};
