/*
 * SPDX-FileCopyrightText: (C) 2025 a dinosaur
 * SPDX-License-Identifier: Zlib
 */

#include "nehe.h"
#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"


#define FONT_ATLAS_W 256
#define FONT_ATLAS_H 256


static SDL_GPUTexture* fontTex = NULL;
static stbtt_bakedchar glyphs[96];

static float projection[16];

static float counter1 = 0.0f, counter2 = 0.0f;


static void NeHe_DrawText(SDL_GPUCommandBuffer* restrict cmd, SDL_GPUTexture* restrict swapchain, float x, float y, const unsigned char* restrict text)
{
	for (int c = *text; c != '\0'; c = (int)*(++text))
	{
		stbtt_aligned_quad quad;
		stbtt_GetBakedQuad(glyphs, FONT_ATLAS_W, FONT_ATLAS_H, c - 0x20, &x, &y, &quad, 1);

		if (c < 0x20 || c > 0x7F)
			continue;

		if (quad.s0 == quad.s1 || quad.t0 == quad.t1 || quad.x0 == quad.x1 || quad.y0 == quad.y1)
			continue;

		const float scale = 1.0f;
		const SDL_GPUBlitInfo blitInfo =
		{
			.source =
			{
				.texture = fontTex,
				.x = (unsigned)(quad.s0 * FONT_ATLAS_W),
				.y = (unsigned)(quad.t0 * FONT_ATLAS_H),
				.w = (unsigned)((quad.s1 - quad.s0) * FONT_ATLAS_W),
				.h = (unsigned)((quad.t1 - quad.t0) * FONT_ATLAS_H)
			},
			.destination =
			{
				.texture = swapchain,
				.x = (unsigned)(scale * (quad.x0)),
				.y = (unsigned)(scale * (quad.y0)),
				.w = (unsigned)(scale * (quad.x1 - quad.x0)),
				.h = (unsigned)(scale * (quad.y1 - quad.y0))
			},
			.load_op = SDL_GPU_LOADOP_LOAD,
			.filter = SDL_GPU_FILTER_LINEAR
		};
		SDL_BlitGPUTexture(cmd, &blitInfo);
	}
}

static void NeHe_Printf(SDL_GPUCommandBuffer* restrict cmd, SDL_GPUTexture* restrict swapchain, float x, float y, const char* restrict fmt, ...)
{
	char text[256];
	va_list args;

	va_start(args, fmt);
		SDL_vsnprintf(text, sizeof(text), fmt, args);
	va_end(args);

	NeHe_DrawText(cmd, swapchain, x, y, (unsigned char*)text);
}

static bool Lesson13_Init(NeHeContext* ctx)
{
	void* ttf = NeHe_ReadResourceBlob(ctx, "Data/NimbusMonoPS-Bold.ttf", NULL);
	if (!ttf)
	{
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to read font file: %s", SDL_GetError());
		return false;
	}

	uint8_t pixels[FONT_ATLAS_W * FONT_ATLAS_H];

	//FW_BOLD is 700
	stbtt_BakeFontBitmap(ttf, 0, 24, pixels, FONT_ATLAS_W, FONT_ATLAS_H, ' ', 96, glyphs);
	SDL_free(ttf);

	const SDL_GPUTextureCreateInfo fontInfo =
	{
		.type = SDL_GPU_TEXTURETYPE_2D,
		.format = SDL_GPU_TEXTUREFORMAT_R8_UNORM,  //SDL_GPU_TEXTUREFORMAT_A8_UNORM
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

static void Lesson13_Quit(NeHeContext* ctx)
{
	//SDL_ReleaseGPUSampler(ctx->device, sampler);
	SDL_ReleaseGPUTexture(ctx->device, fontTex);
	//SDL_ReleaseGPUGraphicsPipeline(ctx->device, pso);
}

static void Lesson13_Resize(NeHeContext* restrict ctx, int width, int height)
{
	(void)ctx;

	// Avoid division by zero by clamping height
	height = SDL_max(height, 1);
	// Recalculate projection matrix
	Mtx_Perspective(projection, 45.0f, (float)width / (float)height, 0.1f, 100.0f);
}

static void Lesson13_Draw(NeHeContext* restrict ctx, SDL_GPUCommandBuffer* restrict cmd, SDL_GPUTexture* restrict swapchain)
{
	(void)ctx;

	const SDL_GPUColorTargetInfo colorInfo =
	{
		.texture = swapchain,
		.clear_color = { 0.0f, 0.0f, 0.0f, 0.5f },
		.load_op = SDL_GPU_LOADOP_CLEAR,
		.store_op = SDL_GPU_STOREOP_STORE
	};

	SDL_GPURenderPass* pass = SDL_BeginGPURenderPass(cmd, &colorInfo, 1, NULL);

	float view[16];
	Mtx_Translation(view, 0.0f, 0.0f, -1.0f);

	float r = SDL_max(0.0f, SDL_cosf(counter1));
	float g = SDL_max(0.0f, SDL_sinf(counter2));
	float b = 1.0f - 0.5f * SDL_cosf(counter1 + counter2);

	float rasterX = -0.45f + 0.05f * SDL_cosf(counter1);
	float rasterY = 0.35f * SDL_sinf(counter2);

	SDL_EndGPURenderPass(pass);

	rasterX += 10.0f;
	rasterY += 20.0f;
	NeHe_Printf(cmd, swapchain, rasterX, rasterY, "Active OpenGL Text With NeHe - %7.2f", counter1);

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
