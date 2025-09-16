/*
 * SPDX-FileCopyrightText: (C) 2025 a dinosaur
 * SPDX-License-Identifier: Zlib
 */

#include "nehe.h"


typedef struct { float r, g, b; } Color;
typedef struct
{
	Color color;
	float x, y;
	int charIdx;
} ShaderCharacter;

#define MAX_CHARACTERS 64

typedef struct
{
	ShaderCharacter* characters;
	unsigned remaining;
} CharacterOutput;

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


static SDL_GPUGraphicsPipeline* pso = NULL, * psoText = NULL;
static SDL_GPUBuffer* vtxBuffer = NULL, * idxBuffer = NULL, * charBuffer = NULL;
static SDL_GPUTransferBuffer* charXferBuffer = NULL;
static SDL_GPUSampler* sampler = NULL;
static SDL_GPUTexture* texture = NULL, * fontTex = NULL;

static Mtx projection;

static float counterA = 0.0f, counterB = 0.0f;


static void NeHe_Print(CharacterOutput* restrict characters,
	SDL_Point pos, Color color, const char* restrict text, int font)
{
	font = SDL_clamp(font, 0, 1);
	characters->remaining = SDL_min(MAX_CHARACTERS, characters->remaining);
	color.r = SDL_clamp(color.r, 0.0f, 1.0f);
	color.g = SDL_clamp(color.g, 0.0f, 1.0f);
	color.b = SDL_clamp(color.b, 0.0f, 1.0f);

	while (characters->remaining)
	{
		const int c = *(text)++;
		if (c == '\0')
		{
			return;
		}

		if (c >= 0x20 && c < 0x80)
		{
			characters->characters[MAX_CHARACTERS - characters->remaining--] = (ShaderCharacter)
			{
				.color = color,
				.x = (float)pos.x, .y = (float)pos.y,
				.charIdx = c - 0x20 + font * 0x80
			};

			pos.x += 10;
		}
	}
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

	if (!NeHe_LoadShaders(ctx, &vertexShader, &fragmentShader, "lesson17",
		&(const NeHeShaderProgramCreateInfo){ .vertexUniforms = 1, .fragmentSamplers = 1 }))
	{
		return false;
	}

	const SDL_GPUVertexAttribute characterAttribs[] =
	{
		{
			.location = 0,
			.buffer_slot = 0,
			.format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3,
			.offset = offsetof(ShaderCharacter, color)
		},
		{
			.location = 1,
			.buffer_slot = 0,
			.format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2,
			.offset = offsetof(ShaderCharacter, x)
		},
		{
			.location = 2,
			.buffer_slot = 0,
			.format = SDL_GPU_VERTEXELEMENTFORMAT_INT,
			.offset = offsetof(ShaderCharacter, charIdx)
		}
	};
	psoText = SDL_CreateGPUGraphicsPipeline(ctx->device, &(const SDL_GPUGraphicsPipelineCreateInfo)
	{
		.vertex_shader = vertexShader,
		.fragment_shader = fragmentShader,
		.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLESTRIP,
		.vertex_input_state =
		{
			.vertex_buffer_descriptions = &(const SDL_GPUVertexBufferDescription)
			{
				.slot = 0,
				.pitch = sizeof(ShaderCharacter),
				.input_rate = SDL_GPU_VERTEXINPUTRATE_INSTANCE
			},
			.num_vertex_buffers = 1,
			.vertex_attributes = characterAttribs,
			.num_vertex_attributes = SDL_arraysize(characterAttribs)
		},
		.rasterizer_state =
		{
			.fill_mode = SDL_GPU_FILLMODE_FILL,
			.cull_mode = SDL_GPU_CULLMODE_NONE,
			.front_face = SDL_GPU_FRONTFACE_COUNTER_CLOCKWISE  // Right-handed coordinates
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
	if (!psoText)
	{
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "SDL_CreateGPUGraphicsPipeline: %s", SDL_GetError());
		return false;
	}

	if ((fontTex = NeHe_LoadTexture(ctx, "Data/Font.bmp", true, false)) == NULL ||
		(texture = NeHe_LoadTexture(ctx, "Data/Bumps.bmp", true, false)) == NULL)
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

	// Create GPU & transfer buffers for text characters
	charBuffer = SDL_CreateGPUBuffer(ctx->device, &(const SDL_GPUBufferCreateInfo)
	{
		.usage = SDL_GPU_BUFFERUSAGE_VERTEX,
		.size = sizeof(ShaderCharacter) * MAX_CHARACTERS
	});
	if (!charBuffer)
	{
		return false;
	}
	charXferBuffer = SDL_CreateGPUTransferBuffer(ctx->device, &(const SDL_GPUTransferBufferCreateInfo)
	{
		.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
		.size = sizeof(ShaderCharacter) * MAX_CHARACTERS
	});
	if (!charXferBuffer)
	{
		return false;
	}

	// Create & upload vtx/idx buffers for 3D object
	if (!NeHe_CreateVertexIndexBuffer(ctx, &vtxBuffer, &idxBuffer,
		vertices, sizeof(vertices),
		indices, sizeof(indices)))
	{
		return false;
	}

	return true;
}

static void Lesson17_Quit(NeHeContext* restrict ctx)
{
	SDL_ReleaseGPUBuffer(ctx->device, idxBuffer);
	SDL_ReleaseGPUBuffer(ctx->device, vtxBuffer);
	SDL_ReleaseGPUTransferBuffer(ctx->device, charXferBuffer);
	SDL_ReleaseGPUBuffer(ctx->device, charBuffer);
	SDL_ReleaseGPUSampler(ctx->device, sampler);
	SDL_ReleaseGPUTexture(ctx->device, texture);
	SDL_ReleaseGPUTexture(ctx->device, fontTex);
	SDL_ReleaseGPUGraphicsPipeline(ctx->device, psoText);
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

	// Acquire character buffer to print to
	CharacterOutput characters =
	{
		.characters = (ShaderCharacter*)SDL_MapGPUTransferBuffer(ctx->device, charXferBuffer, true),
		.remaining = MAX_CHARACTERS
	};

	NeHe_Print(&characters, (SDL_Point)
	{
		.x = 280 + (int)(250.0f * SDL_cosf(counterA)),
		.y = 235 + (int)(200.0f * SDL_sinf(counterB))
	}, (Color)
	{
		.r = SDL_cosf(counterA),
		.g = SDL_sinf(counterB),
		.b = 1.0f - 0.5f * SDL_cosf(counterA + counterB)
	}, "NeHe", 0);

	NeHe_Print(&characters, (SDL_Point)
	{
		.x = 280 + (int)(230.0f * SDL_cosf(counterB)),
		.y = 235 + (int)(200.0f * SDL_sinf(counterA))
	}, (Color)
	{
		.r = SDL_sinf(counterB),
		.g = 1.0f - 0.5f * SDL_cosf(counterA + counterB),
		.b = SDL_cosf(counterA)
	}, "OpenGL", 1);

	const Color blue  = { .r = 0.0f, .g = 0.0f, .b = 1.0f };
	const Color white = { .r = 1.0f, .g = 1.0f, .b = 1.0f };
	SDL_Point p = { .x = 240 + (int)(200.0f * SDL_cosf((counterA + counterB) / 5.0f)), .y = 2 };
	NeHe_Print(&characters, p, blue, "Giuseppe D'Agata", 0);
	p.x += 2;
	NeHe_Print(&characters, p, white, "Giuseppe D'Agata", 0);

	// Copy characters to the GPU
	SDL_UnmapGPUTransferBuffer(ctx->device, charXferBuffer);
	SDL_GPUCopyPass* copyPass = SDL_BeginGPUCopyPass(cmd);
	const unsigned numChars = MAX_CHARACTERS - characters.remaining;
	SDL_UploadToGPUBuffer(copyPass, &(const SDL_GPUTransferBufferLocation)
	{
		.transfer_buffer = charXferBuffer,
		.offset = 0
	}, &(const SDL_GPUBufferRegion)
	{
		.buffer = charBuffer,
		.offset = 0,
		.size = sizeof(ShaderCharacter) * numChars
	}, true);
	SDL_EndGPUCopyPass(copyPass);

	// Begin pass & bind pipeline state
	SDL_GPURenderPass* renderPass = SDL_BeginGPURenderPass(cmd, &colorInfo, 1, &depthInfo);
	SDL_BindGPUGraphicsPipeline(renderPass, pso);

	// Bind texture
	SDL_BindGPUFragmentSamplers(renderPass, 0, &(const SDL_GPUTextureSamplerBinding)
	{
		.texture = texture,
		.sampler = sampler
	}, 1);

	// Bind vertex & index buffers
	SDL_BindGPUVertexBuffers(renderPass, 0, &(const SDL_GPUBufferBinding)
	{
		.buffer = vtxBuffer,
		.offset = 0
	}, 1);
	SDL_BindGPUIndexBuffer(renderPass, &(const SDL_GPUBufferBinding)
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
	SDL_DrawGPUIndexedPrimitives(renderPass, SDL_arraysize(indices), 1, 0, 0, 0);

	// Bind text rendering pipeline
	SDL_BindGPUGraphicsPipeline(renderPass, psoText);

	// Bind font texture
	SDL_BindGPUFragmentSamplers(renderPass, 0, &(const SDL_GPUTextureSamplerBinding)
	{
		.texture = fontTex,
		.sampler = sampler
	}, 1);

	// Bind characters buffer
	SDL_BindGPUVertexBuffers(renderPass, 0, &(const SDL_GPUBufferBinding)
	{
		.buffer = charBuffer,
		.offset = 0
	}, 1);

	// Push matrix uniforms
	const Mtx ortho = Mtx_Orthographic2D(0.0f, 640.0f, 0.0f, 480.0f);
	SDL_PushGPUVertexUniformData(cmd, 0, &ortho, sizeof(ortho));

	// Draw characters
	SDL_DrawGPUPrimitives(renderPass, 4, numChars, 0, 0);

	SDL_EndGPURenderPass(renderPass);

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
