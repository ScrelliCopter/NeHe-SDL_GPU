/*
 * SPDX-FileCopyrightText: (C) 2025 a dinosaur
 * SPDX-License-Identifier: Zlib
 */

#include "nehe.h"
#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"


typedef struct
{
	float srcX, srcY, srcW, srcH;
	float dstX, dstY, dstW, dstH;
} ShaderCharacter;


#define MAX_CHARACTERS 255

static SDL_GPUGraphicsPipeline* pso = NULL;
static SDL_GPUBuffer* charBuffer = NULL;
static SDL_GPUTransferBuffer* charXferBuffer = NULL;
static SDL_GPUSampler* sampler = NULL;

static SDL_GPUTexture* fontTex = NULL;
static stbtt_packedchar fontChars[96];

static Mtx perspective, ortho;

// Counters for animating the text
static float counter1 = 0.0f, counter2 = 0.0f;


#define FONT_ATLAS_W 256
#define FONT_ATLAS_H 192

static bool NeHe_BuildFont(NeHeContext* restrict ctx, const char* const restrict ttfResourcePath, int fontSize)
{
	void* ttf = NeHe_ReadResourceBlob(ctx, ttfResourcePath, NULL);
	if (!ttf)
	{
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to read font file: %s", SDL_GetError());
		return false;
	}

	uint8_t pixels[FONT_ATLAS_W * FONT_ATLAS_H];

	//FW_BOLD is 700
	stbtt_pack_context packCtx;
	int res = stbtt_PackBegin(&packCtx, pixels, FONT_ATLAS_W, FONT_ATLAS_H, 0, 1, NULL);
	if (!res) { return false; }
	res = stbtt_PackFontRange(&packCtx, ttf, 0, (float)fontSize, ' ', 96, fontChars);
	if (!res) { return false; }
	SDL_free(ttf);
	stbtt_PackEnd(&packCtx);

	const SDL_GPUTextureCreateInfo fontInfo =
	{
		.type = SDL_GPU_TEXTURETYPE_2D,
		.format = SDL_GPU_TEXTUREFORMAT_A8_UNORM,
		.usage = SDL_GPU_TEXTUREUSAGE_SAMPLER,
		.width = FONT_ATLAS_W, .height = FONT_ATLAS_H, .layer_count_or_depth = 1,
		.num_levels = 1,
		.sample_count = SDL_GPU_SAMPLECOUNT_1,
		.props = 0
	};
	if ((fontTex = NeHe_CreateGPUTextureFromPixels(ctx, pixels, sizeof(pixels), &fontInfo, false)) == NULL)
	{
		return false;
	}

	return true;
}

static unsigned NeHe_DrawText(ShaderCharacter* restrict outChars, float x, float y, const unsigned char* restrict text)
{
	for (unsigned numChars = 0; numChars < MAX_CHARACTERS;)
	{
		const int c = *(text++);
		if (c == '\0')
		{
			return numChars;
		}

		if (c >= 0x20 && c < 0x80)
		{
			stbtt_aligned_quad quad;
			stbtt_GetPackedQuad(fontChars, FONT_ATLAS_W, FONT_ATLAS_H, c - 0x20, &x, &y, &quad, 1);

			outChars[numChars++] = (ShaderCharacter)
			{
				.srcX = quad.s0, .srcY =  quad.t0,
				.srcW = quad.s1 - quad.s0, .srcH = quad.t1 - quad.t0,
				.dstX = quad.x0, .dstY = -quad.y0,
				.dstW = quad.x1 - quad.x0, .dstH = quad.y0 - quad.y1
			};
		}
	}
	return MAX_CHARACTERS;
}

static unsigned NeHe_Printf(ShaderCharacter* restrict outChars, SDL_PRINTF_FORMAT_STRING const char* restrict fmt, ...)
{
	char text[MAX_CHARACTERS + 1];
	va_list args;

	va_start(args, fmt);
		SDL_vsnprintf(text, sizeof(text), fmt, args);
	va_end(args);

	return NeHe_DrawText(outChars, 0, 0, (unsigned char*)text);
}

static bool Lesson13_Init(NeHeContext* ctx)
{
	SDL_GPUShader* vertexShader, * fragmentShader;
	if (!NeHe_LoadShaders(ctx, &vertexShader, &fragmentShader, "lesson13",
		&(const NeHeShaderProgramCreateInfo){ .vertexUniforms = 1, .fragmentSamplers = 1 }))
	{
		return false;
	}

	const SDL_GPUVertexAttribute vertexAttributes[] =
	{
		{
			.location = 0,
			.buffer_slot = 0,
			.format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT4,
			.offset = offsetof(ShaderCharacter, srcX)
		},
		{
			.location = 1,
			.buffer_slot = 0,
			.format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT4,
			.offset = offsetof(ShaderCharacter, dstX)
		}
	};
	pso = SDL_CreateGPUGraphicsPipeline(ctx->device, &(const SDL_GPUGraphicsPipelineCreateInfo)
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
			.vertex_attributes = vertexAttributes,
			.num_vertex_attributes = SDL_arraysize(vertexAttributes)
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
					.src_color_blendfactor = SDL_GPU_BLENDFACTOR_ONE,
					.dst_color_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA,
					.src_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE,
					.dst_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA
				}
			},
			.num_color_targets = 1
		}
	});

	if (!NeHe_BuildFont(ctx, "Data/NimbusMonoPS-Bold.ttf", 24))
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

	return true;
}

static void Lesson13_Quit(NeHeContext* ctx)
{
	SDL_ReleaseGPUTransferBuffer(ctx->device, charXferBuffer);
	SDL_ReleaseGPUBuffer(ctx->device, charBuffer);
	SDL_ReleaseGPUSampler(ctx->device, sampler);
	SDL_ReleaseGPUTexture(ctx->device, fontTex);
	SDL_ReleaseGPUGraphicsPipeline(ctx->device, pso);
}

static void Lesson13_Resize(NeHeContext* restrict ctx, int width, int height)
{
	(void)ctx;

	// Avoid division by zero by clamping height
	height = SDL_max(height, 1);
	// Recalculate projection matrix
	ortho = Mtx_Orthographic2D(0.0f, (float)width, 0.0f, (float)height);
	perspective = Mtx_Perspective(45.0f, (float)width / (float)height, 0.1f, 100.0f);
}

static void Lesson13_Draw(NeHeContext* restrict ctx, SDL_GPUCommandBuffer* restrict cmd,
	SDL_GPUTexture* restrict swapchain, unsigned w, unsigned h)
{
	const SDL_GPUColorTargetInfo colorInfo =
	{
		.texture = swapchain,
		.clear_color = { 0.0f, 0.0f, 0.0f, 0.5f },
		.load_op = SDL_GPU_LOADOP_CLEAR,
		.store_op = SDL_GPU_STOREOP_STORE
	};

	// Print text to character buffer
	ShaderCharacter* characters = SDL_MapGPUTransferBuffer(ctx->device, charXferBuffer, true);
	unsigned numChars = NeHe_Printf(characters, "Active OpenGL Text With NeHe - %7.2f", counter1);
	SDL_UnmapGPUTransferBuffer(ctx->device, charXferBuffer);

	// Copy characters to the GPU
	SDL_GPUCopyPass* copyPass = SDL_BeginGPUCopyPass(cmd);
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
	SDL_GPURenderPass* renderPass = SDL_BeginGPURenderPass(cmd, &colorInfo, 1, NULL);
	SDL_BindGPUGraphicsPipeline(renderPass, pso);

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

	// Text colour
	float r = SDL_max(0.0f, SDL_cosf(counter1));
	float g = SDL_max(0.0f, SDL_sinf(counter2));
	float b = 1.0f - 0.5f * SDL_cosf(counter1 + counter2);

	// Text position in world space
	const Vec4f textWorldPos =
	{
		0.05f * SDL_cosf(counter1) - 0.45f,
		0.32f * SDL_sinf(counter2),
		-1.0f,
		1.0f
	};

	// Position text in screen coordinates (Y-up)
	Vec4f textScreenPos = Mtx_VectorProject(&perspective, textWorldPos);
	Mtx model = Mtx_Translation(
		floorf((float)w * (textScreenPos.x + 1.0f) / 2.0f),
		floorf((float)h * (textScreenPos.y + 1.0f) / 2.0f),
		0.0f);

	// Push matrix uniforms
	struct Uniform { Mtx modelViewProj; float color[4]; } u =
	{
		.modelViewProj = Mtx_Multiply(&ortho, &model),
		.color = { r, g, b, 1.0f }
	};
	SDL_PushGPUVertexUniformData(cmd, 0, &u, sizeof(u));

	// Draw characters
	SDL_DrawGPUPrimitives(renderPass, 4, numChars, 0, 0);

	SDL_EndGPURenderPass(renderPass);

	counter1 += 0.051f;
	counter2 += 0.005f;
}


const struct AppConfig appConfig =
{
	.title = "NeHe's Bitmap Font Tutorial",
	.width = 640, .height = 480,
	.init = Lesson13_Init,
	.quit = Lesson13_Quit,
	.resize = Lesson13_Resize,
	.draw = Lesson13_Draw
};
