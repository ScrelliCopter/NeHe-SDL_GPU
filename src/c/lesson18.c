/*
 * SPDX-FileCopyrightText: (C) 2025 a dinosaur
 * SPDX-License-Identifier: Zlib
 */

#include "nehe.h"
#include "quad.h"


static const QuadVertexNormalTexture cubeVertices[] =
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

static const uint16_t cubeIndices[] =
{
	 0,  1,  2,   2,  3,  0,  // Front
	 4,  5,  6,   6,  7,  4,  // Back
	 8,  9, 10,  10, 11,  8,  // Top
	12, 13, 14,  14, 15, 12,  // Bottom
	16, 17, 18,  18, 19, 16,  // Right
	20, 21, 22,  22, 23, 20   // Left
};

#define QUADRIC_VERTEX_CAPACITY 1089
#define QUADRIC_INDEX_CAPACITY  0x1800

static enum Object
{
	OBJECT_CUBE,
	OBJECT_CYLINDER,
	OBJECT_DISC,
	OBJECT_SPHERE,
	OBJECT_CONE,
	OBJECT_DYNAMIC,

	NUM_OBJECTS
} object = OBJECT_CUBE;

static SDL_GPUGraphicsPipeline* psoUnlit = NULL, * psoLight = NULL;
static SDL_GPUBuffer* objVtxBuffers[NUM_OBJECTS], * objIdxBuffers[NUM_OBJECTS];
static unsigned objIdxCounts[NUM_OBJECTS];
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


static bool Lesson18_Init(NeHeContext* restrict ctx)
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
			.offset = offsetof(QuadVertexNormalTexture, x)
		},
		{
			.location = 1,
			.buffer_slot = 0,
			.format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2,
			.offset = offsetof(QuadVertexNormalTexture, u)
		},
		{
			.location = 2,
			.buffer_slot = 0,
			.format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3,
			.offset = offsetof(QuadVertexNormalTexture, nx)
		}
	};

	const SDL_GPUVertexInputState vertexInput =
	{
		.vertex_buffer_descriptions = &(const SDL_GPUVertexBufferDescription)
		{
			.slot = 0,
			.pitch = sizeof(QuadVertexNormalTexture),
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

	if ((texture = NeHe_LoadTexture(ctx, "Data/Wall.bmp", true, true)) == NULL)
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

	SDL_zeroa(objVtxBuffers);
	SDL_zeroa(objIdxBuffers);
	SDL_zeroa(objIdxCounts);

	// Upload pre-made cube
	if (!NeHe_CreateVertexIndexBuffer(ctx, &objVtxBuffers[OBJECT_CUBE], &objIdxBuffers[OBJECT_CUBE],
		cubeVertices, sizeof(cubeVertices),
		cubeIndices, sizeof(cubeIndices)))
	{
		return false;
	}
	objIdxCounts[OBJECT_CUBE] = SDL_arraysize(cubeIndices);

	// Pre-generate static quadratics
	QuadVertexNormalTexture quadricVertices[QUADRIC_VERTEX_CAPACITY];
	QuadIndex quadricIndices[QUADRIC_INDEX_CAPACITY];
	Quadric quadratic =
	{
		.vertexData = quadricVertices,
		.indices    = quadricIndices,
		.vertexCapacity = SDL_arraysize(quadricVertices),
		.indexCapacity  = SDL_arraysize(quadricIndices)
	};
	Quad_Cylinder(&quadratic, 1.0f, 1.0f, 3.0f, 32, 32);
	objIdxCounts[OBJECT_CYLINDER] = quadratic.numIndices;
	if (!NeHe_CreateVertexIndexBuffer(ctx, &objVtxBuffers[OBJECT_CYLINDER], &objIdxBuffers[OBJECT_CYLINDER],
		quadratic.vertexData, sizeof(QuadVertexNormalTexture) * quadratic.numVertices,
		quadratic.indices, sizeof(QuadIndex) * quadratic.numIndices))
	{
		return false;
	}
	Quad_Disc(&quadratic, 0.5f, 1.5f, 32, 32);
	objIdxCounts[OBJECT_DISC] = quadratic.numIndices;
	if (!NeHe_CreateVertexIndexBuffer(ctx, &objVtxBuffers[OBJECT_DISC], &objIdxBuffers[OBJECT_DISC],
		quadratic.vertexData, sizeof(QuadVertexNormalTexture) * quadratic.numVertices,
		quadratic.indices, sizeof(QuadIndex) * quadratic.numIndices))
	{
		return false;
	}
	Quad_Cylinder(&quadratic, 1.0f, 0.0f, 3.0f, 32, 32);
	objIdxCounts[OBJECT_CONE] = quadratic.numIndices;
	if (!NeHe_CreateVertexIndexBuffer(ctx, &objVtxBuffers[OBJECT_CONE], &objIdxBuffers[OBJECT_CONE],
		quadratic.vertexData, sizeof(QuadVertexNormalTexture) * quadratic.numVertices,
		quadratic.indices, sizeof(QuadIndex) * quadratic.numIndices))
	{
		return false;
	}
	Quad_Sphere(&quadratic, 1.3f, 32, 32);
	objIdxCounts[OBJECT_SPHERE] = quadratic.numIndices;
	if (!NeHe_CreateVertexIndexBuffer(ctx, &objVtxBuffers[OBJECT_SPHERE], &objIdxBuffers[OBJECT_SPHERE],
		quadratic.vertexData, sizeof(QuadVertexNormalTexture) * quadratic.numVertices,
		quadratic.indices, sizeof(QuadIndex) * quadratic.numIndices))
	{
		return false;
	}

	return true;
}

static void Lesson18_Quit(NeHeContext* restrict ctx)
{
	for (int i = NUM_OBJECTS - 1; i > 0; --i)
	{
		SDL_ReleaseGPUBuffer(ctx->device, objIdxBuffers[i]);
		SDL_ReleaseGPUBuffer(ctx->device, objVtxBuffers[i]);
	}
	for (int i = SDL_arraysize(samplers) - 1; i > 0; --i)
	{
		SDL_ReleaseGPUSampler(ctx->device, samplers[i]);
	}
	SDL_ReleaseGPUTexture(ctx->device, texture);
	SDL_ReleaseGPUGraphicsPipeline(ctx->device, psoLight);
	SDL_ReleaseGPUGraphicsPipeline(ctx->device, psoUnlit);
}

static void Lesson18_Resize(NeHeContext* restrict ctx, int width, int height)
{
	(void)ctx;

	// Avoid division by zero by clamping height
	height = SDL_max(height, 1);
	// Recalculate projection matrix
	projection = Mtx_Perspective(45.0f, (float)width / (float)height, 0.1f, 100.0f);
}

static void Lesson18_Draw(NeHeContext* restrict ctx, SDL_GPUCommandBuffer* restrict cmd,
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

	// Setup the model matrix
	Mtx model = Mtx_Translation(0.0f, 0.0f, z);
	Mtx_Rotate(&model, xRot, 1.0f, 0.0f, 0.0f);
	Mtx_Rotate(&model, yRot, 0.0f, 1.0f, 0.0f);
	if (object == OBJECT_CYLINDER || object == OBJECT_CONE)
	{
		// Centre cylinder & cone
		Mtx_Translate(&model, 0.0f, 0.0f, -1.5f);
	}

	// Bind vertex & index buffers
	const SDL_GPUBufferBinding vtxBinding = { objVtxBuffers[object], 0 };
	const SDL_GPUBufferBinding idxBinding = { objIdxBuffers[object], 0 };
	if (vtxBinding.buffer != NULL)
	{
		SDL_BindGPUVertexBuffers(pass, 0, &vtxBinding, 1);
	}
	if (idxBinding.buffer != NULL)
	{
		SDL_BindGPUIndexBuffer(pass, &idxBinding, (object == OBJECT_CUBE)
			? SDL_GPU_INDEXELEMENTSIZE_16BIT
			: SDL_GPU_INDEXELEMENTSIZE_32BIT);
	}
	unsigned numIndices = objIdxCounts[object];

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

	// Draw object
	if (vtxBinding.buffer && idxBinding.buffer)
	{
		SDL_DrawGPUIndexedPrimitives(pass, numIndices, 1, 0, 0, 0);
	}

	SDL_EndGPURenderPass(pass);

	const bool* keys = SDL_GetKeyboardState(NULL);

	if (keys[SDL_SCANCODE_PAGEUP])   { z -= 0.02f; }
	if (keys[SDL_SCANCODE_PAGEDOWN]) { z += 0.02f; }
	if (keys[SDL_SCANCODE_UP])   { xSpeed -= 0.01f; }
	if (keys[SDL_SCANCODE_DOWN]) { xSpeed += 0.01f; }
	if (keys[SDL_SCANCODE_RIGHT]) { ySpeed += 0.01f; }
	if (keys[SDL_SCANCODE_LEFT])  { ySpeed -= 0.01f; }

	xRot += xSpeed;
	yRot += ySpeed;
}

static void Lesson18_Key(NeHeContext* ctx, SDL_Keycode key, bool down, bool repeat)
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
		case SDLK_SPACE:
			object = (object + 1) % NUM_OBJECTS;
			break;
		default:
			break;
		}
	}
}


const struct AppConfig appConfig =
{
	.title = "NeHe & TipTup's Quadratics Tutorial",
	.width = 640, .height = 480,
	.createDepthFormat = SDL_GPU_TEXTUREFORMAT_D16_UNORM,
	.init = Lesson18_Init,
	.quit = Lesson18_Quit,
	.resize = Lesson18_Resize,
	.draw = Lesson18_Draw,
	.key = Lesson18_Key
};
