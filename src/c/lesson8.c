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
static SDL_GPUGraphicsPipeline* psoBlendUnlit = NULL, * psoBlendLight = NULL;
static SDL_GPUBuffer* vtxBuffer = NULL;
static SDL_GPUBuffer* idxBuffer = NULL;
static SDL_GPUSampler* samplers[3] = { NULL, NULL, NULL };
static SDL_GPUTexture* texture = NULL;

static float projection[16];

static bool lighting = false;
static bool blending = false;
struct Light { float ambient[4], diffuse[4], position[4]; } static light =
{
	.ambient  = { 0.5f, 0.5f, 0.5f, 1.0f },
	.diffuse  = { 1.0f, 1.0f, 1.0f, 1.0f },
	.position = { 0.0f, 0.0f, 2.0f, 1.0f }
};

static int filter = 0;

static float xRot = 0.0f, yRot = 0.0f;
static float xSpeed = 0.0f, ySpeed = 0.0f;
static float z = -5.0f;


static bool Lesson8_Init(NeHeContext* restrict ctx)
{
	SDL_GPUShader* vertexShaderUnlit, * fragmentShaderUnlit;
	SDL_GPUShader* vertexShaderLight, * fragmentShaderLight;
	if (!NeHe_LoadShaders(ctx, &vertexShaderUnlit, &fragmentShaderUnlit, "lesson8",
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

	SDL_GPUGraphicsPipelineCreateInfo psoInfo;
	SDL_zero(psoInfo);

	psoInfo.primitive_type     = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;
	psoInfo.vertex_input_state = (SDL_GPUVertexInputState)
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

	psoInfo.rasterizer_state = (SDL_GPURasterizerState)
	{
		.fill_mode = SDL_GPU_FILLMODE_FILL,
		.cull_mode = SDL_GPU_CULLMODE_NONE,
		.front_face = SDL_GPU_FRONTFACE_COUNTER_CLOCKWISE
	};

	psoInfo.target_info.num_color_targets        = 1;
	psoInfo.target_info.depth_stencil_format     = appConfig.createDepthFormat;
	psoInfo.target_info.has_depth_stencil_target = true;

	// Common pipeline depth & colour target options
	psoInfo.depth_stencil_state.compare_op = SDL_GPU_COMPAREOP_LESS_OR_EQUAL;
	const SDL_GPUTextureFormat swapchainTextureFormat = SDL_GetGPUSwapchainTextureFormat(ctx->device, ctx->window);

	// Setup depth/stencil & colour pipeline state for no blending
	psoInfo.depth_stencil_state.enable_depth_test  = true;
	psoInfo.depth_stencil_state.enable_depth_write = true;
	psoInfo.target_info.color_target_descriptions  = &(const SDL_GPUColorTargetDescription)
	{
		.format = swapchainTextureFormat
	};

	// Create unlit pipeline
	psoInfo.vertex_shader   = vertexShaderUnlit;
	psoInfo.fragment_shader = fragmentShaderUnlit;
	psoUnlit = SDL_CreateGPUGraphicsPipeline(ctx->device, &psoInfo);

	// Create lit pipeline
	psoInfo.vertex_shader   = vertexShaderLight;
	psoInfo.fragment_shader = fragmentShaderLight;
	psoLight = SDL_CreateGPUGraphicsPipeline(ctx->device, &psoInfo);

	// Setup depth/stencil & colour pipeline state for blending
	psoInfo.depth_stencil_state.enable_depth_test  = false;
	psoInfo.depth_stencil_state.enable_depth_write = false;
	psoInfo.target_info.color_target_descriptions  = &(const SDL_GPUColorTargetDescription)
	{
		.format = swapchainTextureFormat,
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
	};

	// Create unlit blended pipeline
	psoInfo.vertex_shader   = vertexShaderUnlit;
	psoInfo.fragment_shader = fragmentShaderUnlit;
	psoBlendUnlit = SDL_CreateGPUGraphicsPipeline(ctx->device, &psoInfo);

	// Create lit blended pipeline
	psoInfo.vertex_shader   = vertexShaderLight;
	psoInfo.fragment_shader = fragmentShaderLight;
	psoBlendLight = SDL_CreateGPUGraphicsPipeline(ctx->device, &psoInfo);

	// Free shaders
	SDL_ReleaseGPUShader(ctx->device, fragmentShaderLight);
	SDL_ReleaseGPUShader(ctx->device, vertexShaderLight);
	SDL_ReleaseGPUShader(ctx->device, fragmentShaderUnlit);
	SDL_ReleaseGPUShader(ctx->device, vertexShaderUnlit);

	if (!psoUnlit || !psoLight || !psoBlendUnlit || !psoBlendLight)
	{
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "SDL_CreateGPUGraphicsPipeline: %s", SDL_GetError());
		return false;
	}

	if ((texture = NeHe_LoadTexture(ctx, "Data/Glass.bmp", true, true)) == NULL)
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

static void Lesson8_Quit(NeHeContext* restrict ctx)
{
	SDL_ReleaseGPUBuffer(ctx->device, idxBuffer);
	SDL_ReleaseGPUBuffer(ctx->device, vtxBuffer);
	for (int i = SDL_arraysize(samplers) - 1; i > 0; --i)
	{
		SDL_ReleaseGPUSampler(ctx->device, samplers[i]);
	}
	SDL_ReleaseGPUTexture(ctx->device, texture);
	SDL_ReleaseGPUGraphicsPipeline(ctx->device, psoBlendLight);
	SDL_ReleaseGPUGraphicsPipeline(ctx->device, psoBlendUnlit);
	SDL_ReleaseGPUGraphicsPipeline(ctx->device, psoLight);
	SDL_ReleaseGPUGraphicsPipeline(ctx->device, psoUnlit);
}

static void Lesson8_Resize(NeHeContext* restrict ctx, int width, int height)
{
	(void)ctx;

	// Avoid division by zero by clamping height
	height = SDL_max(height, 1);
	// Recalculate projection matrix
	Mtx_Perspective(projection, 45.0f, (float)width / (float)height, 0.1f, 100.0f);
}

static void Lesson8_Draw(NeHeContext* restrict ctx, SDL_GPUCommandBuffer* restrict cmd, SDL_GPUTexture* restrict swapchain)
{
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
	SDL_GPUGraphicsPipeline* const pipelines[] = { psoUnlit, psoLight, psoBlendUnlit, psoBlendLight };
	SDL_BindGPUGraphicsPipeline(pass, pipelines[lighting + blending * 2]);

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

	// Setup the view
	float model[16];
	Mtx_Translation(model, 0.0f, 0.0f, z);
	Mtx_Rotate(model, xRot, 1.0f, 0.0f, 0.0f);
	Mtx_Rotate(model, yRot, 0.0f, 1.0f, 0.0f);

	// Push shader uniforms
	if (lighting)
	{
		struct { float model[16], projection[16]; } u;
		SDL_memcpy(u.model, model, sizeof(u.model));
		SDL_memcpy(u.projection, projection, sizeof(u.projection));
		SDL_PushGPUVertexUniformData(cmd, 0, &u, sizeof(u));
		SDL_PushGPUVertexUniformData(cmd, 1, &light, sizeof(light));
	}
	else
	{
		struct { float modelViewProj[16], color[4]; } u;
		Mtx_Multiply(u.modelViewProj, projection, model);
		// 50% translucency
		SDL_memcpy(u.color, (const float[4]){ 1.0f, 1.0f, 1.0f, 0.5f }, sizeof(float) * 4);
		SDL_PushGPUVertexUniformData(cmd, 0, &u, sizeof(u));
	}

	// Draw textured cube
	SDL_DrawGPUIndexedPrimitives(pass, SDL_arraysize(indices), 1, 0, 0, 0);

	SDL_EndGPURenderPass(pass);

	const bool* keys = SDL_GetKeyboardState(NULL);

	if (keys[SDL_SCANCODE_UP])   { xSpeed -= 0.01f; }
	if (keys[SDL_SCANCODE_DOWN]) { xSpeed += 0.01f; }
	if (keys[SDL_SCANCODE_RIGHT]) { ySpeed += 0.1f; }
	if (keys[SDL_SCANCODE_LEFT])  { ySpeed -= 0.1f; }
	if (keys[SDL_SCANCODE_PAGEUP])   { z -= 0.02f; }
	if (keys[SDL_SCANCODE_PAGEDOWN]) { z += 0.02f; }

	xRot += xSpeed;
	yRot += ySpeed;
}

static void Lesson8_Key(NeHeContext* ctx, SDL_Keycode key, bool down, bool repeat)
{
	(void)ctx;

	if (down && !repeat)
	{
		switch (key)
		{
		case SDLK_L:
			lighting = !lighting;
			break;
		case SDLK_B:
			blending = !blending;
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
	.title = "Tom Stanis & NeHe's Blending Tutorial",
	.width = 640, .height = 480,
	.createDepthFormat = SDL_GPU_TEXTUREFORMAT_D16_UNORM,
	.init = Lesson8_Init,
	.quit = Lesson8_Quit,
	.resize = Lesson8_Resize,
	.draw = Lesson8_Draw,
	.key = Lesson8_Key
};
