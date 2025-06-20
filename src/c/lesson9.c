/*
 * SPDX-FileCopyrightText: (C) 2025 a dinosaur
 * SPDX-License-Identifier: Zlib
 */

#include "nehe.h"


typedef struct
{
	float x, y, z;
	float u, v;
} Vertex;

static const Vertex vertices[] =
{
	{ -1.0f, -1.0f, 0.0f,  0.0f, 0.0f },
	{  1.0f, -1.0f, 0.0f,  1.0f, 0.0f },
	{  1.0f,  1.0f, 0.0f,  1.0f, 1.0f },
	{ -1.0f,  1.0f, 0.0f,  0.0f, 1.0f }
};

static const uint16_t indices[] =
{
	0,  1,  2,
	2,  3,  0
};

static SDL_GPUGraphicsPipeline* pso = NULL;
static SDL_GPUBuffer* vtxBuffer = NULL;
static SDL_GPUBuffer* idxBuffer = NULL;
static SDL_GPUTexture* texture = NULL;
static SDL_GPUSampler* sampler = NULL;

static SDL_GPUBuffer* instanceBuffer = NULL;
static SDL_GPUTransferBuffer* instanceXferBuffer = NULL;

typedef struct
{
	float model[16];
	float color[4];
} Instance;

static float projection[16];

static bool twinkle = false;

static struct Star
{
	float distance, angle;
	uint8_t r, g, b;
} stars[50];

static float zoom = -15.0f;
static float tilt = 90.0f;
static float spin = 0.0f;


static bool Lesson9_Init(NeHeContext* ctx)
{
	SDL_GPUShader* vertexShader, * fragmentShader;
	if (!NeHe_LoadShaders(ctx, &vertexShader, &fragmentShader, "lesson9",
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
			.format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2,
			.offset = offsetof(Vertex, u)
		},
		// Instance matrix attributes (one for each column)
		{
			.location = 2,
			.buffer_slot = 1,
			.format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT4,
			.offset = (uint32_t)offsetof(Instance, model)
		},
		{
			.location = 3,
			.buffer_slot = 1,
			.format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT4,
			.offset = (uint32_t)offsetof(Instance, model[4])
		},
		{
			.location = 4,
			.buffer_slot = 1,
			.format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT4,
			.offset = (uint32_t)offsetof(Instance, model[8])
		},
		{
			.location = 5,
			.buffer_slot = 1,
			.format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT4,
			.offset = (uint32_t)offsetof(Instance, model[12])
		},
		// Instance colour
		{
			.location = 6,
			.buffer_slot = 1,
			.format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT4,
			.offset = offsetof(Instance, color)
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
		.target_info =
		{
			.color_target_descriptions = &(const SDL_GPUColorTargetDescription)
			{
				.format = SDL_GetGPUSwapchainTextureFormat(ctx->device, ctx->window),
				.blend_state =
				{
					.enable_blend = true,
					.color_blend_op = SDL_GPU_BLENDOP_ADD,
					.alpha_blend_op = SDL_GPU_BLENDOP_ADD,
					.src_color_blendfactor = SDL_GPU_BLENDFACTOR_SRC_ALPHA,
					.dst_color_blendfactor = SDL_GPU_BLENDFACTOR_ONE,
					.src_alpha_blendfactor = SDL_GPU_BLENDFACTOR_SRC_ALPHA,
					.dst_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE
				}
			},
			.num_color_targets = 1
		}
	});
	SDL_ReleaseGPUShader(ctx->device, fragmentShader);
	SDL_ReleaseGPUShader(ctx->device, vertexShader);
	if (!pso)
	{
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "SDL_CreateGPUGraphicsPipeline: %s", SDL_GetError());
		return false;
	}

	if ((texture = NeHe_LoadTexture(ctx, "Data/Star.bmp", true, false)) == NULL)
	{
		return false;
	}

	sampler = SDL_CreateGPUSampler(ctx->device, &(const SDL_GPUSamplerCreateInfo)
	{
		.mag_filter = SDL_GPU_FILTER_LINEAR,
		.min_filter = SDL_GPU_FILTER_LINEAR
	});
	if (!sampler)
	{
		return false;
	}

	if (!NeHe_CreateVertexIndexBuffer(ctx, &vtxBuffer, &idxBuffer,
		vertices, sizeof(vertices),
		indices, sizeof(indices)))
	{
		return false;
	}

	const int numStars = SDL_arraysize(stars);

	instanceBuffer = SDL_CreateGPUBuffer(ctx->device, &(const SDL_GPUBufferCreateInfo)
	{
		.usage = SDL_GPU_BUFFERUSAGE_VERTEX,
		.size = sizeof(Instance) * 2 * numStars
	});
	if (!instanceBuffer)
	{
		return false;
	}
	instanceXferBuffer = SDL_CreateGPUTransferBuffer(ctx->device, &(const SDL_GPUTransferBufferCreateInfo)
	{
		.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
		.size = sizeof(Instance) * 2 * numStars
	});
	if (!instanceXferBuffer)
	{
		return false;
	}

	// Initialise stars
	for (int i = 0; i < numStars; ++i)
	{
		stars[i] = (struct Star)
		{
			.angle = 0.0f,
			.distance = 5.0f * ((float)i / (float)numStars),
			.r = (uint8_t)(NeHe_Random() % 256),
			.g = (uint8_t)(NeHe_Random() % 256),
			.b = (uint8_t)(NeHe_Random() % 256)
		};
	}

	return true;
}

static void Lesson9_Quit(NeHeContext* ctx)
{
	SDL_ReleaseGPUTransferBuffer(ctx->device, instanceXferBuffer);
	SDL_ReleaseGPUBuffer(ctx->device, instanceBuffer);
	SDL_ReleaseGPUBuffer(ctx->device, idxBuffer);
	SDL_ReleaseGPUBuffer(ctx->device, vtxBuffer);
	SDL_ReleaseGPUSampler(ctx->device, sampler);
	SDL_ReleaseGPUTexture(ctx->device, texture);
	SDL_ReleaseGPUGraphicsPipeline(ctx->device, pso);
}

static void Lesson9_Resize(NeHeContext* ctx, int width, int height)
{
	(void)ctx;

	// Avoid division by zero by clamping height
	height = SDL_max(height, 1);
	// Recalculate projection matrix
	Mtx_Perspective(projection, 45.0f, (float)width / (float)height, 0.1f, 100.0f);
}

static void Lesson9_Draw(NeHeContext* restrict ctx, SDL_GPUCommandBuffer* restrict cmd, SDL_GPUTexture* restrict swapchain)
{
	const SDL_GPUColorTargetInfo colorInfo =
	{
		.texture = swapchain,
		.clear_color = { 0.0f, 0.0f, 0.0f, 0.5f },
		.load_op = SDL_GPU_LOADOP_CLEAR,
		.store_op = SDL_GPU_STOREOP_STORE
	};

	static const int numStars = SDL_arraysize(stars);

	// Animate stars
	Instance* instances = SDL_MapGPUTransferBuffer(ctx->device, instanceXferBuffer, true);
	for (int i = 0, instanceIdx = 0; i < numStars; ++i, ++instanceIdx)
	{
		struct Star* star = &stars[i];
		Instance* instance = &instances[instanceIdx];

		Mtx_Translation(instance->model, 0.0f ,0.0f, zoom);
		Mtx_Rotate(instance->model, tilt, 1.0f, 0.0f, 0.0f);
		Mtx_Rotate(instance->model, star->angle, 0.0f, 1.0f, 0.0f);
		Mtx_Translate(instance->model, star->distance, 0.0f, 0.0f);
		Mtx_Rotate(instance->model, -star->angle, 0.0f, 1.0f, 0.0f);
		Mtx_Rotate(instance->model, -tilt, 1.0f, 0.0f, 0.0f);

		if (twinkle)
		{
			instance->color[0] = (float)stars[numStars - i - 1].r / 255.0f;
			instance->color[1] = (float)stars[numStars - i - 1].g / 255.0f;
			instance->color[2] = (float)stars[numStars - i - 1].b / 255.0f;
			instance->color[3] = 1.0f;
			SDL_memcpy(instances[++instanceIdx].model, instance->model, sizeof(float) * 16);
			instance = &instances[instanceIdx];
		}

		Mtx_Rotate(instance->model, spin, 0.0f, 0.0f, 1.0f);
		instance->color[0] = (float)star->r / 255.0f;
		instance->color[1] = (float)star->g / 255.0f;
		instance->color[2] = (float)star->b / 255.0f;
		instance->color[3] = 1.0f;

		spin += 0.01f;
		star->angle += (float)i / (float)numStars;
		star->distance -= 0.01f;
		if (star->distance < 0.0f)
		{
			star->distance += 5.0f;
			star->r = (uint8_t)(NeHe_Random() % 256);
			star->g = (uint8_t)(NeHe_Random() % 256);
			star->b = (uint8_t)(NeHe_Random() % 256);
		}
	}
	SDL_UnmapGPUTransferBuffer(ctx->device, instanceXferBuffer);

	const unsigned numInstances = twinkle ? 2 * (unsigned)numStars : (unsigned)numStars;

	// Upload instances buffer to the GPU
	SDL_GPUCopyPass* copyPass = SDL_BeginGPUCopyPass(cmd);
	SDL_UploadToGPUBuffer(copyPass, &(const SDL_GPUTransferBufferLocation)
	{
		.transfer_buffer = instanceXferBuffer,
		.offset = 0
	}, &(const SDL_GPUBufferRegion)
	{
		.buffer = instanceBuffer,
		.offset = 0,
		.size = sizeof(Instance) * numInstances
	}, true);
	SDL_EndGPUCopyPass(copyPass);

	// Begin pass & bind pipeline state
	SDL_GPURenderPass* renderPass = SDL_BeginGPURenderPass(cmd, &colorInfo, 1, NULL);
	SDL_BindGPUGraphicsPipeline(renderPass, pso);

	// Bind particle texture
	SDL_BindGPUFragmentSamplers(renderPass, 0, &(const SDL_GPUTextureSamplerBinding)
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
	SDL_BindGPUVertexBuffers(renderPass, 0, vertexBindings, SDL_arraysize(vertexBindings));
	SDL_BindGPUIndexBuffer(renderPass, &(const SDL_GPUBufferBinding)
	{
		.buffer = idxBuffer,
		.offset = 0
	}, SDL_GPU_INDEXELEMENTSIZE_16BIT);

	SDL_PushGPUVertexUniformData(cmd, 0, projection, sizeof(projection));
	SDL_DrawGPUIndexedPrimitives(renderPass, SDL_arraysize(indices), numInstances, 0, 0, 0);

	SDL_EndGPURenderPass(renderPass);

	const bool* keys = SDL_GetKeyboardState(NULL);

	if (keys[SDL_SCANCODE_UP])       { tilt -= 0.5f; }
	if (keys[SDL_SCANCODE_DOWN])     { tilt += 0.5f; }
	if (keys[SDL_SCANCODE_PAGEUP])   { zoom -= 0.2f; }
	if (keys[SDL_SCANCODE_PAGEDOWN]) { zoom += 0.2f; }
}

static void Lesson9_Key(NeHeContext* ctx, SDL_Keycode key, bool down, bool repeat)
{
	(void)ctx;

	if (down && !repeat)
	{
		switch (key)
		{
		case SDLK_T:
			twinkle = !twinkle;
			break;
		default:
			break;
		}
	}
}


const struct AppConfig appConfig =
{
	.title = "NeHe's Animated Blended Textures Tutorial",
	.width = 640, .height = 480,
	.init = Lesson9_Init,
	.quit = Lesson9_Quit,
	.resize = Lesson9_Resize,
	.draw = Lesson9_Draw,
	.key = Lesson9_Key
};
