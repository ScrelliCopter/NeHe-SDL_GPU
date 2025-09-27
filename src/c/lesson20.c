/*
 * SPDX-FileCopyrightText: (C) 2025 a dinosaur
 * SPDX-License-Identifier: Zlib
 */

#include "nehe.h"


typedef struct
{
	Mtx modelViewProj;
	float texOffsetX, texOffsetY;
	float texScaleX, texScaleY;
} VertexUniform;


static SDL_GPUGraphicsPipeline* pso = NULL, * psoMask = NULL, * psoBlend = NULL;
static SDL_GPUTexture* textureLogo = NULL;
static SDL_GPUTexture* textureMask1 = NULL, * textureMask2 = NULL;
static SDL_GPUTexture* textureImage1 = NULL, * textureImage2 = NULL;
static SDL_GPUSampler* sampler = NULL;

static Mtx projection;


static bool masking = true;
static int scene = 0;

static float animate = 0.0f;


static bool Lesson20_Init(NeHeContext* restrict ctx)
{
	SDL_GPUShader* vertexShader, * fragmentShader;
	if (!NeHe_LoadShaders(ctx, &vertexShader, &fragmentShader, "lesson20",
		&(const NeHeShaderProgramCreateInfo){ .vertexUniforms = 1, .fragmentSamplers = 1 }))
	{
		return false;
	}

	SDL_GPUColorTargetDescription colorDesc =
	{
		.format = SDL_GetGPUSwapchainTextureFormat(ctx->device, ctx->window)
	};
	SDL_GPUGraphicsPipelineCreateInfo psoInfo =
	{
		.vertex_shader = vertexShader,
		.fragment_shader = fragmentShader,
		.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLESTRIP,
		.rasterizer_state =
		{
			.fill_mode = SDL_GPU_FILLMODE_FILL,
			.cull_mode = SDL_GPU_CULLMODE_NONE,
			.front_face = SDL_GPU_FRONTFACE_COUNTER_CLOCKWISE  // Right-handed coordinates
		},
		.target_info =
		{
			.color_target_descriptions = &colorDesc,
			.num_color_targets = 1
		}
	};

	// Create opaque pipeline
	if ((pso = SDL_CreateGPUGraphicsPipeline(ctx->device, &psoInfo)) == NULL)
	{
		SDL_ReleaseGPUShader(ctx->device, fragmentShader);
		SDL_ReleaseGPUShader(ctx->device, vertexShader);
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "SDL_CreateGPUGraphicsPipeline: %s", SDL_GetError());
		return false;
	}

	// Create masking pipeline
	colorDesc.blend_state = (SDL_GPUColorTargetBlendState)
	{
		.enable_blend = true,
		.color_blend_op = SDL_GPU_BLENDOP_ADD,
		.alpha_blend_op = SDL_GPU_BLENDOP_ADD,
		.src_color_blendfactor = SDL_GPU_BLENDFACTOR_DST_COLOR,
		.dst_color_blendfactor = SDL_GPU_BLENDFACTOR_ZERO,
		.src_alpha_blendfactor = SDL_GPU_BLENDFACTOR_DST_COLOR,
		.dst_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ZERO
	};
	if ((psoMask = SDL_CreateGPUGraphicsPipeline(ctx->device, &psoInfo)) == NULL)
	{
		SDL_ReleaseGPUShader(ctx->device, fragmentShader);
		SDL_ReleaseGPUShader(ctx->device, vertexShader);
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "SDL_CreateGPUGraphicsPipeline: %s", SDL_GetError());
		return false;
	}

	// Create additive blending pipeline
	colorDesc.blend_state = (SDL_GPUColorTargetBlendState)
	{
		.enable_blend = true,
		.color_blend_op = SDL_GPU_BLENDOP_ADD,
		.alpha_blend_op = SDL_GPU_BLENDOP_ADD,
		.src_color_blendfactor = SDL_GPU_BLENDFACTOR_ONE,
		.dst_color_blendfactor = SDL_GPU_BLENDFACTOR_ONE,
		.src_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE,
		.dst_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE
	};
	psoBlend = SDL_CreateGPUGraphicsPipeline(ctx->device, &psoInfo);
	SDL_ReleaseGPUShader(ctx->device, fragmentShader);
	SDL_ReleaseGPUShader(ctx->device, vertexShader);
	if (!psoBlend)
	{
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "SDL_CreateGPUGraphicsPipeline: %s", SDL_GetError());
		return false;
	}

	// Create & upload textures
	if ((textureLogo   = NeHe_LoadTexture(ctx, "Data/Logo.bmp",   true, false)) == NULL ||
		(textureMask1  = NeHe_LoadTexture(ctx, "Data/Mask1.bmp",  true, false)) == NULL ||
		(textureImage1 = NeHe_LoadTexture(ctx, "Data/Image1.bmp", true, false)) == NULL ||
		(textureMask2  = NeHe_LoadTexture(ctx, "Data/Mask2.bmp",  true, false)) == NULL ||
		(textureImage2 = NeHe_LoadTexture(ctx, "Data/Image2.bmp", true, false)) == NULL)
	{
		return false;
	}

	// Create texture sampler (linear)
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

	return true;
}

static void Lesson20_Quit(NeHeContext* restrict ctx)
{
	SDL_ReleaseGPUSampler(ctx->device, sampler);
	SDL_ReleaseGPUTexture(ctx->device, textureImage2);
	SDL_ReleaseGPUTexture(ctx->device, textureMask2);
	SDL_ReleaseGPUTexture(ctx->device, textureImage1);
	SDL_ReleaseGPUTexture(ctx->device, textureMask1);
	SDL_ReleaseGPUTexture(ctx->device, textureLogo);
	SDL_ReleaseGPUGraphicsPipeline(ctx->device, psoBlend);
	SDL_ReleaseGPUGraphicsPipeline(ctx->device, psoMask);
	SDL_ReleaseGPUGraphicsPipeline(ctx->device, pso);
}

static void Lesson20_Resize(NeHeContext* restrict ctx, int width, int height)
{
	(void)ctx;

	// Avoid division by zero by clamping height
	height = SDL_max(height, 1);
	// Recalculate projection matrix
	projection = Mtx_Perspective(45.0f, (float)width / (float)height, 0.1f, 100.0f);
}

static void Lesson20_Draw(NeHeContext* restrict ctx, SDL_GPUCommandBuffer* restrict cmd,
	SDL_GPUTexture* restrict swapchain, unsigned swapchainW, unsigned swapchainH)
{
	(void)ctx; (void)swapchainW; (void)swapchainH;

	const SDL_GPUColorTargetInfo colorInfo =
	{
		.texture = swapchain,
		.clear_color = { 0.0f, 0.0f, 0.0f, 0.0f },
		.load_op = SDL_GPU_LOADOP_CLEAR,
		.store_op = SDL_GPU_STOREOP_STORE
	};

	// Begin pass
	SDL_GPURenderPass* pass = SDL_BeginGPURenderPass(cmd, &colorInfo, 1, NULL);

	Mtx model = Mtx_Translation(0.0f, 0.0f, -2.0f);

	// Draw logo background
	SDL_BindGPUGraphicsPipeline(pass, pso);  // Opaque
	SDL_BindGPUFragmentSamplers(pass, 0, &(const SDL_GPUTextureSamplerBinding)
	{
		.texture = textureLogo,
		.sampler = sampler
	}, 1);
	VertexUniform u =
	{
		.modelViewProj = Mtx_Multiply(&projection, &model),
		.texOffsetX = 0.0f,
		.texOffsetY = -animate,
		.texScaleX = 3.0f,
		.texScaleY = 3.0f
	};
	SDL_PushGPUVertexUniformData(cmd, 0, &u, sizeof(VertexUniform));
	SDL_DrawGPUPrimitives(pass, 4, 1, 0, 0);

	// Setup overlay uniforms
	if (scene == 0)
	{
		u = (VertexUniform)
		{
			.modelViewProj = u.modelViewProj,  // Reuse background matrix
			.texOffsetX = animate,
			.texOffsetY = 0.0f,
			.texScaleX = 4.0f,
			.texScaleY = 4.0f,
		};
	}
	else
	{
		// Rotate around centre and move further into screen
		Mtx_Translate(&model, 0.0f, 0.0f, -1.0f);
		Mtx_Rotate(&model, 360.0f * animate, 0.0f, 0.0f, 1.0f);

		u = (VertexUniform)
		{
			.modelViewProj = Mtx_Multiply(&projection, &model),
			.texOffsetX = 0.0f,
			.texOffsetY = 0.0f,
			.texScaleX = 1.0f,
			.texScaleY = 1.0f,
		};
	}
	SDL_PushGPUVertexUniformData(cmd, 0, &u, sizeof(VertexUniform));

	// Draw overlay mask
	if (masking)
	{
		SDL_BindGPUGraphicsPipeline(pass, psoMask);  // Masking
		SDL_BindGPUFragmentSamplers(pass, 0, &(const SDL_GPUTextureSamplerBinding)
		{
			.texture = (scene == 0) ? textureMask1 : textureMask2,
			.sampler = sampler
		}, 1);
	SDL_DrawGPUPrimitives(pass, 4, 1, 0, 0);
	}

	// Draw overlay
	SDL_BindGPUGraphicsPipeline(pass, psoBlend);  // Additive blending
	SDL_BindGPUFragmentSamplers(pass, 0, &(const SDL_GPUTextureSamplerBinding)
	{
		.texture = (scene == 0) ? textureImage1 : textureImage2,
		.sampler = sampler
	}, 1);
	SDL_DrawGPUPrimitives(pass, 4, 1, 0, 0);

	SDL_EndGPURenderPass(pass);

	animate = SDL_fmodf(animate + 0.002f, 1.0f);
}

static void Lesson20_Key(NeHeContext* ctx, SDL_Keycode key, bool down, bool repeat)
{
	(void)ctx;

	if (down && !repeat)
	{
		switch (key)
		{
		case SDLK_SPACE:
			scene = !scene;
			break;
		case SDLK_M:
			masking = !masking;
			break;
		default:
			break;
		}
	}
}


const struct AppConfig appConfig =
{
	.title = "NeHe's Masking Tutorial",
	.width = 640, .height = 480,
	.init = Lesson20_Init,
	.quit = Lesson20_Quit,
	.resize = Lesson20_Resize,
	.draw = Lesson20_Draw,
	.key = Lesson20_Key
};
