/*
 * SPDX-FileCopyrightText: (C) 2025 a dinosaur
 * SPDX-License-Identifier: Zlib
 */

#include "nehe.h"


typedef struct
{
	float srcX, srcY, srcW, srcH;
	float dstX, dstY, dstW, dstH;
} ShaderCharacter;

#define MAX_CHARACTERS 255

typedef struct
{
	float x, y, z;
	float u, v;
} Vertex;

#define SQRT2 1.4142135f  // sqrt(2)

static const Vertex vertices[6] =
{
	{ -SQRT2,   0.0f,   0.0f,  0.0f, 0.0f },
	{   0.0f,  SQRT2,   0.0f,  1.0f, 0.0f },
	{  SQRT2,   0.0f,   0.0f,  1.0f, 1.0f },
	{   0.0f, -SQRT2,   0.0f,  0.0f, 1.0f },
	{   0.0f,   0.0f,  SQRT2,  0.0f, 0.0f },
	{   0.0f,   0.0f, -SQRT2,  1.0f, 1.0f }
};

static const uint16_t indices[12] =
{
	0, 1, 2, 2, 3, 0,
	3, 4, 1, 1, 5, 3
};


static SDL_GPUGraphicsPipeline* pso = NULL;
static SDL_GPUBuffer* vtxBuffer = NULL;
static SDL_GPUBuffer* idxBuffer = NULL;
static SDL_GPUSampler* sampler = NULL;
static SDL_GPUTexture* texture = NULL;

static Mtx projection, ortho;

static float counterA = 0.0f, counterB = 0.0f;


static unsigned NeHe_Print(ShaderCharacter* restrict outChars,
	SDL_Point pos, SDL_FColor color, const char* restrict text, int font)
{
	for (unsigned numChars = 0; numChars < MAX_CHARACTERS;)
	{
		const int c = *(text)++;
		if (c == '\0')
		{
			return numChars;
		}

		if (c >= 0x20 && c < 0x80)
		{
			const int charIdx = c - 0x20 + font * 128;
			const int sheetY = charIdx / 16;
			const int sheetX = charIdx - sheetY * 16;

			outChars[numChars++] = (ShaderCharacter)
			{
				.srcX = 0.0625f * (float)sheetX,
				.srcY = 0.9375f - 0.0625f * (float)sheetY,
				.srcW = 0.0625f, .srcH = 0.0625f,
				.dstX = (float)pos.x, .dstY = (float)pos.y,
				.dstW = 16.0f, .dstH = 16.0f
			};

			pos.x += 10;
		}
	}
	return MAX_CHARACTERS;
}


static bool Lesson17_Init(NeHeContext* restrict ctx)
{
	SDL_GPUShader* vertexShader, * fragmentShader;
	if (!NeHe_LoadShaders(ctx, &vertexShader, &fragmentShader, "lesson6",
		&(const NeHeShaderProgramCreateInfo){ .vertexUniforms = 1, .fragmentSamplers = 1 }))
	{
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
		}
	};
	pso = SDL_CreateGPUGraphicsPipeline(ctx->device, &(const SDL_GPUGraphicsPipelineCreateInfo)
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

	//glBlendFunc(GL_SRC_ALPHA, GL_ONE);

	//if ((texture = NeHe_LoadTexture(ctx, "Data/Font.bmp", true, false)) == NULL)
	if ((texture = NeHe_LoadTexture(ctx, "Data/Bumps.bmp", true, false)) == NULL)
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

	ortho = Mtx_Orthographic2D(0.0f, 640.0f, 0.0f, 480.0f);

	return true;
}

static void Lesson17_Quit(NeHeContext* restrict ctx)
{
	SDL_ReleaseGPUBuffer(ctx->device, idxBuffer);
	SDL_ReleaseGPUBuffer(ctx->device, vtxBuffer);
	SDL_ReleaseGPUSampler(ctx->device, sampler);
	SDL_ReleaseGPUTexture(ctx->device, texture);
	SDL_ReleaseGPUGraphicsPipeline(ctx->device, pso);
}

static void Lesson17_Resize(NeHeContext* restrict ctx, int width, int height)
{
	(void)ctx;

	// Avoid division by zero by clamping height
	height = SDL_max(height, 1);
	// Recalculate projection matrix
	projection = Mtx_Perspective(45.0f, (float)width / (float)height, 0.1f, 100.0f);
}

static void Lesson17_Draw(NeHeContext* restrict ctx, SDL_GPUCommandBuffer* restrict cmd,
	SDL_GPUTexture* restrict swapchain, unsigned swapchainW, unsigned swapchainH)
{
	(void)swapchainW; (void)swapchainH;

	const SDL_GPUColorTargetInfo colorInfo =
	{
		.texture = swapchain,
		.clear_color = { 0.0f, 0.0f, 0.0f, 0.0f },
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
	SDL_BindGPUGraphicsPipeline(pass, pso);

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

	// Move 5 units into the screen and spin
	Mtx model = Mtx_Translation(0.0f, 0.0f, -5.0f);
	Mtx_Rotate(&model, 30.0f * counterA, 0.0f, 1.0f, 0.0f);

	// Push shader uniforms
	Mtx modelViewProj = Mtx_Multiply(&projection, &model);
	SDL_PushGPUVertexUniformData(cmd, 0, &modelViewProj, sizeof(Mtx));

	// Draw textured 3D object
	SDL_DrawGPUIndexedPrimitives(pass, SDL_arraysize(indices), 1, 0, 0, 0);

	/*
	ShaderCharacter chars[MAX_CHARACTERS];

	NeHe_Print(chars, (SDL_Point)
	{
		.x = 280 + (int)(250.0f * SDL_cosf(counterA)),
		.y = 235 + (int)(200.0f * SDL_sinf(counterB))
	}, (SDL_FColor)
	{
		.r = SDL_max(0.0f, SDL_cosf(counterA)),
		.g = SDL_max(0.0f, SDL_sinf(counterB)),
		.b = 1.0f - 0.5f * SDL_cosf(counterA + counterB),
		.a = 1.0f,
	}, "NeHe", 0);

	NeHe_Print(chars, (SDL_Point)
	{
		.x = 280 + (int)(230.0f * SDL_cosf(counterB)),
		.y = 235 + (int)(200.0f * SDL_sinf(counterA))
	},
	(SDL_FColor)
	{
		.r = SDL_max(0.0f, SDL_sinf(counterB)),
		.g = 1.0f - 0.5f * SDL_cosf(counterA + counterB),
		.b = SDL_max(0.0f, SDL_cosf(counterA)),
		.a = 1.0f
	}, "OpenGL", 1);

	const SDL_FColor blue  = { .r = 0.0f, .g = 0.0f, .b = 1.0f, .a = 1.0f };
	const SDL_FColor white = { .r = 1.0f, .g = 1.0f, .b = 1.0f, .a = 1.0f };
	SDL_Point p = { .x = 240 + (int)(200.0f * SDL_cosf(counterA + counterB) / 5.0f), .y = 2 };
	NeHe_Print(chars, p, blue, "Giuseppe D'Agata", 0);
	p.x += 2;
	NeHe_Print(chars, p, white, "Giuseppe D'Agata", 0);
	*/

	SDL_EndGPURenderPass(pass);

	counterA += 0.01f;
	counterB += 0.0081f;
}

const struct AppConfig appConfig =
{
	.title = "NeHe & Giuseppe D'Agata's 2D Font Tutorial",
	.width = 640, .height = 480,
	.createDepthFormat = SDL_GPU_TEXTUREFORMAT_D16_UNORM,
	.init = Lesson17_Init,
	.quit = Lesson17_Quit,
	.resize = Lesson17_Resize,
	.draw = Lesson17_Draw
};
