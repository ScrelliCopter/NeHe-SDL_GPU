/*
 * SPDX-FileCopyrightText: (C) 2025 a dinosaur
 * SPDX-License-Identifier: Zlib
 */

#include "nehe.h"


typedef struct
{
	float x, y, z;
	float nx, ny, nz;
	float u, v;
	float tint;
} Vertex;

typedef struct
{
	float model[16];
	float r, g, b, a;
} Instance;

static const Vertex vertices[] =
{
	// Bottom face
	{ -1.0f, -1.0f, -1.0f,   0.0f, -1.0f,  0.0f,  1.0f, 1.0f,  1.0f },
	{  1.0f, -1.0f, -1.0f,   0.0f, -1.0f,  0.0f,  0.0f, 1.0f,  1.0f },
	{  1.0f, -1.0f,  1.0f,   0.0f, -1.0f,  0.0f,  0.0f, 0.0f,  1.0f },
	{ -1.0f, -1.0f,  1.0f,   0.0f, -1.0f,  0.0f,  1.0f, 0.0f,  1.0f },
	// Front face
	{ -1.0f, -1.0f,  1.0f,   0.0f,  0.0f,  1.0f,  0.0f, 0.0f,  1.0f },
	{  1.0f, -1.0f,  1.0f,   0.0f,  0.0f,  1.0f,  1.0f, 0.0f,  1.0f },
	{  1.0f,  1.0f,  1.0f,   0.0f,  0.0f,  1.0f,  1.0f, 1.0f,  1.0f },
	{ -1.0f,  1.0f,  1.0f,   0.0f,  0.0f,  1.0f,  0.0f, 1.0f,  1.0f },
	// Back face
	{ -1.0f, -1.0f, -1.0f,   0.0f,  0.0f, -1.0f,  1.0f, 0.0f,  1.0f },
	{ -1.0f,  1.0f, -1.0f,   0.0f,  0.0f, -1.0f,  1.0f, 1.0f,  1.0f },
	{  1.0f,  1.0f, -1.0f,   0.0f,  0.0f, -1.0f,  0.0f, 1.0f,  1.0f },
	{  1.0f, -1.0f, -1.0f,   0.0f,  0.0f, -1.0f,  0.0f, 0.0f,  1.0f },
	// Right face
	{  1.0f, -1.0f, -1.0f,   1.0f,  0.0f,  0.0f,  1.0f, 0.0f,  1.0f },
	{  1.0f,  1.0f, -1.0f,   1.0f,  0.0f,  0.0f,  1.0f, 1.0f,  1.0f },
	{  1.0f,  1.0f,  1.0f,   1.0f,  0.0f,  0.0f,  0.0f, 1.0f,  1.0f },
	{  1.0f, -1.0f,  1.0f,   1.0f,  0.0f,  0.0f,  0.0f, 0.0f,  1.0f },
	// Left face
	{ -1.0f, -1.0f, -1.0f,  -1.0f,  0.0f,  0.0f,  0.0f, 0.0f,  1.0f },
	{ -1.0f, -1.0f,  1.0f,  -1.0f,  0.0f,  0.0f,  1.0f, 0.0f,  1.0f },
	{ -1.0f,  1.0f,  1.0f,  -1.0f,  0.0f,  0.0f,  1.0f, 1.0f,  1.0f },
	{ -1.0f,  1.0f, -1.0f,  -1.0f,  0.0f,  0.0f,  0.0f, 1.0f,  1.0f },
	// Top face
	{ -1.0f,  1.0f, -1.0f,   0.0f,  1.0f,  0.0f,  0.0f, 1.0f,  0.5f },
	{ -1.0f,  1.0f,  1.0f,   0.0f,  1.0f,  0.0f,  0.0f, 0.0f,  0.5f },
	{  1.0f,  1.0f,  1.0f,   0.0f,  1.0f,  0.0f,  1.0f, 0.0f,  0.5f },
	{  1.0f,  1.0f, -1.0f,   0.0f,  1.0f,  0.0f,  1.0f, 1.0f,  0.5f }
};

static const uint16_t indices[] =
{
	 0,  1,  2,   2,  3,  0,  // Bottom face
	 4,  5,  6,   6,  7,  4,  // Front face
	 8,  9, 10,  10, 11,  8,  // Back face
	12, 13, 14,  14, 15, 12,  // Right face
	16, 17, 18,  18, 19, 16,  // Left face
	20, 21, 22,  22, 23, 20   // Top face
};


static SDL_GPUGraphicsPipeline* pso = NULL;
static SDL_GPUBuffer* vtxBuffer = NULL;
static SDL_GPUBuffer* idxBuffer = NULL;
static SDL_GPUBuffer* instanceBuffer = NULL;
static SDL_GPUTransferBuffer* instanceXferBuffer = NULL;
static SDL_GPUSampler* sampler = NULL;
static SDL_GPUTexture* texture = NULL;

static float projection[16];

static float xRot = 0.0f, yRot = 0.0f;
static float z = -20.0f;

static const int numRows = 5;
#define NUM_INSTANCES (numRows * (numRows + 1) / 2)  // Triangular number


static bool Lesson12_Init(NeHeContext* restrict ctx)
{
	SDL_GPUShader* vertexShader, * fragmentShader;
	if (!NeHe_LoadShaders(ctx, &vertexShader, &fragmentShader, "lesson12",
		&(const NeHeShaderProgramCreateInfo){ .vertexUniforms = 1, .fragmentSamplers = 1 }))
	{
		return false;
	}

	const SDL_GPUVertexAttribute vertexAttribs[] =
	{
		// Mesh attributes
		{
			.location = 0,
			.buffer_slot = 0,
			.format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3,
			.offset = offsetof(Vertex, x)
		},
		{
			.location = 1,
			.buffer_slot = 0,
			.format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3,
			.offset = offsetof(Vertex, nx)
		},
		{
			.location = 2,
			.buffer_slot = 0,
			.format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3,
			.offset = offsetof(Vertex, u)
		},
		{
			.location = 3,
			.buffer_slot = 0,
			.format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT,
			.offset = offsetof(Vertex, tint)
		},
		// Instance matrix attributes (one for each column)
		{
			.location = 4,
			.buffer_slot = 1,
			.format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT4,
			.offset = offsetof(Instance, model)
		},
		{
			.location = 5,
			.buffer_slot = 1,
			.format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT4,
			.offset = offsetof(Instance, model[4])
		},
		{
			.location = 6,
			.buffer_slot = 1,
			.format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT4,
			.offset = offsetof(Instance, model[8])
		},
		{
			.location = 7,
			.buffer_slot = 1,
			.format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT4,
			.offset = offsetof(Instance, model[12])
		},
		// Instance colour
		{
			.location = 8,
			.buffer_slot = 1,
			.format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT4,
			.offset = offsetof(Instance, r)
		}
	};
	const SDL_GPUVertexBufferDescription bufferDescriptors[] =
	{
		// Slot for mesh
		{
			.slot = 0,
			.pitch = sizeof(Vertex),
			.input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX
		},
		// Slot for instances
		{
			.slot = 1,
			.pitch = sizeof(Instance),
			.input_rate = SDL_GPU_VERTEXINPUTRATE_INSTANCE
		}
	};
	pso = SDL_CreateGPUGraphicsPipeline(ctx->device, &(const SDL_GPUGraphicsPipelineCreateInfo)
	{
		.vertex_shader = vertexShader,
		.fragment_shader = fragmentShader,
		.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST,
		.vertex_input_state =
		{
			.vertex_buffer_descriptions = bufferDescriptors,
			.num_vertex_buffers = SDL_arraysize(bufferDescriptors),
			.vertex_attributes = vertexAttribs,
			.num_vertex_attributes = SDL_arraysize(vertexAttribs)
		},
		.rasterizer_state =
		{
			.fill_mode = SDL_GPU_FILLMODE_FILL,
			.cull_mode = SDL_GPU_CULLMODE_NONE,
			.front_face = SDL_GPU_FRONTFACE_COUNTER_CLOCKWISE,  // Right-handed coordinates
			.enable_depth_clip = true  // OpenGL-like clip behaviour
		},
		.depth_stencil_state =
		{
			.compare_op = SDL_GPU_COMPAREOP_LESS_OR_EQUAL,
			.enable_depth_test = true,
			.enable_depth_write = true
		},
		.target_info =
		{
			.color_target_descriptions = &(const SDL_GPUColorTargetDescription)
			{
				.format = SDL_GetGPUSwapchainTextureFormat(ctx->device, ctx->window)
			},
			.num_color_targets = 1,
			.depth_stencil_format = appConfig.createDepthFormat,
			.has_depth_stencil_target = true
		}
	});
	SDL_ReleaseGPUShader(ctx->device, fragmentShader);
	SDL_ReleaseGPUShader(ctx->device, vertexShader);
	if (!pso)
	{
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "SDL_CreateGPUGraphicsPipeline: %s", SDL_GetError());
		return false;
	}

	if ((texture = NeHe_LoadTexture(ctx, "Data/Cube.bmp", true, false)) == NULL)
	{
		return false;
	}

	sampler = SDL_CreateGPUSampler(ctx->device, &(const SDL_GPUSamplerCreateInfo)
	{
		.min_filter = SDL_GPU_FILTER_LINEAR,
		.mag_filter = SDL_GPU_FILTER_LINEAR
	});
	if (!sampler)
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

	instanceBuffer = SDL_CreateGPUBuffer(ctx->device, &(const SDL_GPUBufferCreateInfo)
	{
		.usage = SDL_GPU_BUFFERUSAGE_GRAPHICS_STORAGE_READ,
		.size = sizeof(Instance) * NUM_INSTANCES
	});
	if (!instanceBuffer)
	{
		return false;
	}
	instanceXferBuffer = SDL_CreateGPUTransferBuffer(ctx->device, &(const SDL_GPUTransferBufferCreateInfo)
	{
		.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
		.size = sizeof(Instance) * NUM_INSTANCES
	});
	if (!instanceXferBuffer)
	{
		return false;
	}

	return true;
}

static void Lesson12_Quit(NeHeContext* restrict ctx)
{
	SDL_ReleaseGPUTransferBuffer(ctx->device, instanceXferBuffer);
	SDL_ReleaseGPUBuffer(ctx->device, instanceBuffer);
	SDL_ReleaseGPUBuffer(ctx->device, idxBuffer);
	SDL_ReleaseGPUBuffer(ctx->device, vtxBuffer);
	SDL_ReleaseGPUSampler(ctx->device, sampler);
	SDL_ReleaseGPUTexture(ctx->device, texture);
	SDL_ReleaseGPUGraphicsPipeline(ctx->device, pso);
}

static void Lesson12_Resize(NeHeContext* restrict ctx, int width, int height)
{
	(void)ctx;

	// Avoid division by zero by clamping height
	height = SDL_max(height, 1);
	// Recalculate projection matrix
	Mtx_Perspective(projection, 45.0f, (float)width / (float)height, 0.1f, 100.0f);
}

static void Lesson12_Draw(NeHeContext* restrict ctx, SDL_GPUCommandBuffer* restrict cmd,
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

	static const float boxColors[5][3] =
	{
		{ 1.0f, 0.0f, 0.0f },  // Red
		{ 1.0f, 0.5f, 0.0f },  // Orange
		{ 1.0f, 1.0f, 0.0f },  // Yellow
		{ 0.0f, 1.0f, 0.0f },  // Green
		{ 0.0f, 1.0f, 1.0f }   // Cyan
	};

	Instance* instances = SDL_MapGPUTransferBuffer(ctx->device, instanceXferBuffer, true);
	for (int row = 0; row < numRows; ++row)
	{
		const float rowFact = (float)(row + 1);
		for (int x = 0; x <= row; ++x)
		{
			Instance* instance = instances++;

			Mtx_Translation(instance->model,
				1.4f + (float)x * 2.8f - rowFact * 1.4f,
				((float)(numRows + 1) - rowFact) * 2.4f - (float)(numRows + 2),
				0);
			Mtx_Rotate(instance->model, 45.0f - 2.0f * rowFact + xRot, 1.0f, 0.0f, 0.0f);
			Mtx_Rotate(instance->model, 45.0f + yRot, 0.0f, 1.0f, 0.0f);

			const int colIdx = SDL_min(row, (int)SDL_arraysize(boxColors) - 1);
			instance->r = boxColors[colIdx][0];
			instance->g = boxColors[colIdx][1];
			instance->b = boxColors[colIdx][2];
			instance->a = 1.0f;
		}
	}
	SDL_UnmapGPUTransferBuffer(ctx->device, instanceXferBuffer);

	// Upload instances to the GPU
	SDL_GPUCopyPass* copyPass = SDL_BeginGPUCopyPass(cmd);
		SDL_UploadToGPUBuffer(copyPass, &(const SDL_GPUTransferBufferLocation)
	{
		.transfer_buffer = instanceXferBuffer,
		.offset = 0
	}, &(const SDL_GPUBufferRegion)
	{
		.buffer = instanceBuffer,
		.offset = 0,
		.size = sizeof(Instance) * NUM_INSTANCES
	}, true);
	SDL_EndGPUCopyPass(copyPass);

	// Begin pass & bind pipeline state
	SDL_GPURenderPass* pass = SDL_BeginGPURenderPass(cmd, &colorInfo, 1, &depthInfo);
	SDL_BindGPUGraphicsPipeline(pass, pso);

	// Bind texture
	SDL_BindGPUFragmentSamplers(pass, 0, &(const SDL_GPUTextureSamplerBinding)
	{
		.texture = texture,
		.sampler = sampler
	}, 1);

	// Bind vertex, instance, and index buffers
	const SDL_GPUBufferBinding vertexBindings[] =
	{
		{ .buffer = vtxBuffer, .offset = 0 },
		{ .buffer = instanceBuffer, .offset = 0 }
	};
	SDL_BindGPUVertexBuffers(pass, 0, vertexBindings, SDL_arraysize(vertexBindings));
	SDL_BindGPUIndexBuffer(pass, &(const SDL_GPUBufferBinding)
	{
		.buffer = idxBuffer,
		.offset = 0
	}, SDL_GPU_INDEXELEMENTSIZE_16BIT);

	// Push shader uniforms
	struct Uniform { float view[16], projection[16]; } u;
	Mtx_Translation(u.view, 0.0f, 0.0f, z);
	SDL_memcpy(u.projection, projection, sizeof(u.projection));
	SDL_PushGPUVertexUniformData(cmd, 0, &u, sizeof(u));

	// Draw textured cube instances
	SDL_DrawGPUIndexedPrimitives(pass, SDL_arraysize(indices), NUM_INSTANCES, 0, 0, 0);

	SDL_EndGPURenderPass(pass);

	const bool* keys = SDL_GetKeyboardState(NULL);

#ifdef NEHE_EXTENDED
	if (keys[SDL_SCANCODE_PAGEUP])   { z -= 0.02f; }
	if (keys[SDL_SCANCODE_PAGEDOWN]) { z += 0.02f; }
	if (keys[SDL_SCANCODE_R])        { xRot = yRot = 0.0f; }
#endif
	if (keys[SDL_SCANCODE_UP])    { xRot -= 0.2f; }
	if (keys[SDL_SCANCODE_DOWN])  { xRot += 0.2f; }
	if (keys[SDL_SCANCODE_LEFT])  { yRot -= 0.2f; }
	if (keys[SDL_SCANCODE_RIGHT]) { yRot += 0.2f; }
}


const struct AppConfig appConfig =
{
	.title = "NeHe's Display List Tutorial",
	.width = 640, .height = 480,
	.createDepthFormat = SDL_GPU_TEXTUREFORMAT_D32_FLOAT,
	.init = Lesson12_Init,
	.quit = Lesson12_Quit,
	.resize = Lesson12_Resize,
	.draw = Lesson12_Draw
};
