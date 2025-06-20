/*
 * SPDX-FileCopyrightText: (C) 2025 a dinosaur
 * SPDX-License-Identifier: Zlib
 */

#include "nehe.h"


typedef struct
{
	float x, y;
	float u, v;
} Vertex;


static SDL_GPUGraphicsPipeline* psoFront = NULL, * psoBack = NULL;
static SDL_GPUBuffer* vtxBuffer = NULL;
static SDL_GPUBuffer* idxBuffer = NULL;
static SDL_GPUSampler* sampler = NULL;
static SDL_GPUTexture* texture = NULL;

static float projection[16];

static const int gridSize = 44;
static const int numGridTris = gridSize * gridSize * 6;
static const int numGridLines = gridSize * (gridSize + 1) * 4;
static const int numIndices = numGridTris + numGridLines;
SDL_COMPILE_TIME_ASSERT(numIndices, numIndices <= UINT16_MAX);
typedef uint16_t index_t;

static int wiggleCount = 0;
static float xRot = 0.0f, yRot = 0.0f, zRot = 0.0f;


static bool Lesson11_Init(NeHeContext* ctx)
{
	SDL_GPUShader* vertexShader, * fragmentShader;
	if (!NeHe_LoadShaders(ctx, &vertexShader, &fragmentShader, "lesson11",
		&(const NeHeShaderProgramCreateInfo){ .vertexUniforms = 1, .fragmentSamplers = 1 }))
	{
		return false;
	}

	const SDL_GPUVertexAttribute vertexAttribs[] =
	{
		{
			.location = 0,
			.buffer_slot = 0,
			.format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2,
			.offset = offsetof(Vertex, x)
		},
		{
			.location = 1,
			.buffer_slot = 0,
			.format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2,
			.offset = offsetof(Vertex, u)
		}
	};
	SDL_GPUGraphicsPipelineCreateInfo psoInfo =
	{
		.vertex_shader = vertexShader,
		.fragment_shader = fragmentShader,
		.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST,
		.vertex_input_state =
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
		},
		.rasterizer_state =
		{
			.fill_mode = SDL_GPU_FILLMODE_FILL,
			.cull_mode = SDL_GPU_CULLMODE_FRONT,
			.front_face = SDL_GPU_FRONTFACE_COUNTER_CLOCKWISE
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
	};
	psoFront = SDL_CreateGPUGraphicsPipeline(ctx->device, &psoInfo);
	psoInfo.primitive_type = SDL_GPU_PRIMITIVETYPE_LINELIST;
	psoInfo.rasterizer_state.fill_mode = SDL_GPU_FILLMODE_LINE;
	//FIXME: The original uses glPolygonMode(GL_FRONT, GL_LINE) for the back of the flag,
	//       but then uses GL_QUADS which we can't use, so we fake it by drawing lines
	//       separately and lose the ability to cull the lines. This probably requires a
	//       compute shader to fix properly, so we'll put up with this inaccuracy for now.
	psoInfo.rasterizer_state.cull_mode = SDL_GPU_CULLMODE_BACK;
	psoBack = SDL_CreateGPUGraphicsPipeline(ctx->device, &psoInfo);
	SDL_ReleaseGPUShader(ctx->device, fragmentShader);
	SDL_ReleaseGPUShader(ctx->device, vertexShader);
	if (!psoFront || !psoBack)
	{
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "SDL_CreateGPUGraphicsPipeline: %s", SDL_GetError());
		return false;
	}

	if ((texture = NeHe_LoadTexture(ctx, "Data/Tim.bmp", true, false)) == NULL)
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

	// Build flag vertices
	Vertex vertices[(gridSize + 1) * (gridSize + 1)];
	const float xyStep = 9.0f / (float)(gridSize + 1);  // Deliberately off-by-one to match behaviour
	const float uvStep = 1.0f / (float)gridSize;
	unsigned i = 0;
	for (int y = 0; y < gridSize + 1; ++y)
	{
		for (int x = 0; x < gridSize + 1; ++x)
		{
			vertices[i++] = (Vertex)
			{
				.x = (float)x * xyStep - 4.5f,
				.y = (float)y * xyStep - 4.5f,
				.u = (float)x * uvStep,
				.v = (float)y * uvStep
			};
		}
	}

	// Build flag indices
	index_t indices[numIndices];
	i = 0;
	for (int y = 0; y < gridSize; ++y)
	{
		const int base = y * (gridSize + 1);
		for (int x = 0; x < gridSize; ++x)
		{
			indices[i++] = (index_t)(base + x);
			indices[i++] = (index_t)(base + x + gridSize + 1);
			indices[i++] = (index_t)(base + x + gridSize + 2);
			indices[i++] = (index_t)(base + x + gridSize + 2);
			indices[i++] = (index_t)(base + x + 1);
			indices[i++] = (index_t)(base + x);
		}
	}
	for (int y = 0; y < gridSize + 1; ++y)
	{
		const int base = y * (gridSize + 1);
		for (int x = 0; x < gridSize; ++x)
		{
			indices[i++] = (index_t)(base + x);
			indices[i++] = (index_t)(base + x + 1);
		}
	}
	for (int x = 0; x < gridSize + 1; ++x)
	{
		for (int y = 0; y < gridSize; ++y)
		{
			indices[i++] = (index_t)(x + (gridSize + 1) * y);
			indices[i++] = (index_t)(x + (gridSize + 1) * (y + 1));
		}
	}

	// Upload flag vertices & indices
	if (!NeHe_CreateVertexIndexBuffer(ctx, &vtxBuffer, &idxBuffer,
		vertices, sizeof(vertices),
		indices, sizeof(indices)))
	{
		return false;
	}

	return true;
}

static void Lesson11_Quit(NeHeContext* ctx)
{
	SDL_ReleaseGPUBuffer(ctx->device, idxBuffer);
	SDL_ReleaseGPUBuffer(ctx->device, vtxBuffer);
	SDL_ReleaseGPUSampler(ctx->device, sampler);
	SDL_ReleaseGPUTexture(ctx->device, texture);
	SDL_ReleaseGPUGraphicsPipeline(ctx->device, psoBack);
	SDL_ReleaseGPUGraphicsPipeline(ctx->device, psoFront);
}

static void Lesson11_Resize(NeHeContext* ctx, int width, int height)
{
	(void)ctx;

	// Avoid division by zero by clamping height
	height = SDL_max(height, 1);
	// Recalculate projection matrix
	Mtx_Perspective(projection, 45.0f, (float)width / (float)height, 0.1f, 100.0f);
}

static void Lesson11_Draw(NeHeContext* restrict ctx, SDL_GPUCommandBuffer* restrict cmd, SDL_GPUTexture* restrict swapchain)
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

	// Begin pass
	SDL_GPURenderPass* pass = SDL_BeginGPURenderPass(cmd, &colorInfo, 1, &depthInfo);

	// Bind texture
	SDL_BindGPUFragmentSamplers(pass, 0, &(const SDL_GPUTextureSamplerBinding)
	{
		.texture = texture,
		.sampler = sampler
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

	float model[16];

	Mtx_Translation(model, 0.0f, 0.0f, -12.0f);
	Mtx_Rotate(model, xRot, 1.0f, 0.0f, 0.0f);
	Mtx_Rotate(model, yRot, 0.0f, 1.0f, 0.0f);
	Mtx_Rotate(model, zRot, 0.0f, 0.0f, 1.0f);

	// Push shader uniforms
	struct Uniform { float modelViewProj[16]; float waveOffset; } u;
	Mtx_Multiply(u.modelViewProj, projection, model);
	u.waveOffset = (float)((wiggleCount / 2) % 45) / 45.0f;  // NOLINT(*-integer-division)
	SDL_PushGPUVertexUniformData(cmd, 0, &u, sizeof(u));

	// Draw textured flag (Front, triangles)
	SDL_BindGPUGraphicsPipeline(pass, psoFront);
	SDL_DrawGPUIndexedPrimitives(pass, numGridTris, 1, 0, 0, 0);

	// Draw textured flag (Back, lines)
	SDL_BindGPUGraphicsPipeline(pass, psoBack);
	SDL_DrawGPUIndexedPrimitives(pass, numGridTris, 1, numGridTris, 0, 0);

	SDL_EndGPURenderPass(pass);

	++wiggleCount;

	xRot += 0.3f;
	yRot += 0.2f;
	zRot += 0.4f;
}


const struct AppConfig appConfig =
{
	.title = "bosco & NeHe's Waving Texture Tutorial",
	.width = 640, .height = 480,
	.createDepthFormat = SDL_GPU_TEXTUREFORMAT_D16_UNORM,
	.init = Lesson11_Init,
	.quit = Lesson11_Quit,
	.resize = Lesson11_Resize,
	.draw = Lesson11_Draw
};
