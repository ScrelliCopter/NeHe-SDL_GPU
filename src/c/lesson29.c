/*
 * SPDX-FileCopyrightText: (C) 2025 a dinosaur
 * SPDX-License-Identifier: Zlib
 */

#include "nehe.h"


static bool ImageReadRAW(const NeHeContext* restrict ctx, SDL_Surface* restrict img,
	const char* const restrict resourcePath)
{
	// Open raw texture file
	char* path = NeHe_ResourcePath(ctx, resourcePath);
	if (!path)
		return false;
	SDL_IOStream* f = SDL_IOFromFile(path, "rb");
	SDL_free(path);
	if (!f)
	{
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "SDL_IOFromFile: %s", SDL_GetError());
		return false;
	}

	// Lock surface for writing
	if (!SDL_LockSurface(img))
	{
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "SDL_LockSurface: %s", SDL_GetError());
		SDL_CloseIO(f);
		return false;
	}

	// Read image data from file from bottom to top, padding with an alpha (255) byte
	const size_t bytesPerPixel = (size_t)SDL_BYTESPERPIXEL(img->format);
	for (int row = img->h - 1; row >= 0; --row)
	{
		uint8_t* rowP = &((uint8_t*)img->pixels)[img->pitch * row];
		for (int col = 0; col < img->w; ++col)
		{
			SDL_ReadIO(f, rowP, bytesPerPixel - 1);
			rowP[bytesPerPixel - 1] = 0xFF;
			rowP += bytesPerPixel;
		}
	}

	SDL_UnlockSurface(img);
	SDL_CloseIO(f);
	return true;
}

static bool ImageBlit(
	SDL_Surface* restrict src, const SDL_Rect srcRect,
	SDL_Surface* restrict dst, const SDL_Point dstOff,
	bool blend, int alpha)
{
	SDL_assert(src->format == dst->format);
	const int bytesPerPixel = SDL_BYTESPERPIXEL(src->format);

	alpha = SDL_clamp(alpha, 0, 0xFF);

	// Lock surfaces for read/write
	if (!SDL_LockSurface(src) || !SDL_LockSurface(dst))
	{
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "SDL_LockSurface: %s", SDL_GetError());
		SDL_UnlockSurface(dst);
		SDL_UnlockSurface(src);
		return false;
	}

	const uint8_t* srcP = &((uint8_t*)src->pixels)[srcRect.y * src->pitch];
	uint8_t* dstP = &((uint8_t*)dst->pixels)[dstOff.y * dst->pitch];

	for (int row = 0; row < srcRect.h; ++row)
	{
		srcP += bytesPerPixel * srcRect.x;
		dstP += bytesPerPixel * dstOff.x;
		for (int col = 0; col < srcRect.w; ++col)
		{
			for (int i = 0; i < bytesPerPixel; ++i, ++dstP)
			{
				uint8_t s = (*srcP++);
				// BUG: The following blending is completely broken, and will always result in channel components
				//      maxing out at 255²/256 = 254 (including the alpha channel!)  The original maths is
				//      preserved in order to match original program behaviour.  If you want to copy this
				//      formula for some reason, you can fix the bug by either adding 1 to alpha, or by swapping
				//      the bit-shift with a /255.  Please just use floats or SDL_BlitSurface instead though.
				(*dstP) = blend ? (uint8_t)((s * alpha + (*dstP) * (0xFF - alpha)) >> 8) : s;
			}
		}
		srcP += bytesPerPixel * (src->w - (srcRect.w + srcRect.x));
		dstP += bytesPerPixel * (dst->w - (srcRect.w + dstOff.x));
	}

	SDL_UnlockSurface(dst);
	SDL_UnlockSurface(src);
	return true;
}

static SDL_GPUTexture* BuildTexture(NeHeContext* restrict ctx)
{
	// Load raw image data into surfaces
	SDL_Surface* imageBg = NULL, * overlay = NULL;
	if ((imageBg = SDL_CreateSurface(256, 256, SDL_PIXELFORMAT_ABGR8888)) == NULL ||
		(overlay = SDL_CreateSurface(256, 256, SDL_PIXELFORMAT_ABGR8888)) == NULL ||
		!ImageReadRAW(ctx, imageBg, "Data/Monitor.raw") ||
		!ImageReadRAW(ctx, overlay, "Data/GL.raw"))
	{
		if (!imageBg || !overlay)
			SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "SDL_CreateSurface: %s", SDL_GetError());
		SDL_DestroySurface(overlay);
		SDL_DestroySurface(imageBg);
		return NULL;
	}

	// Composite overlay onto monitor image
	const bool blitSuccess = ImageBlit(
		overlay, (SDL_Rect){ 127, 127, 128, 128 },
		imageBg, (SDL_Point){ 64, 64 },
		true, 127);
	SDL_DestroySurface(overlay);
	if (!blitSuccess)
	{
		SDL_DestroySurface(imageBg);
		return NULL;
	}

	// Create texture
	SDL_GPUTexture* texture = NeHe_CreateGPUTextureFromSurface(ctx, imageBg, false);
	SDL_DestroySurface(imageBg);
	return texture;
}


typedef struct
{
	float x, y, z;
	float u, v;
} Vertex;

static const Vertex vertices[] =
{
	// Front face
	{  1.0f,  1.0f,  1.0f,  1.0f, 1.0f },
	{ -1.0f,  1.0f,  1.0f,  0.0f, 1.0f },
	{ -1.0f, -1.0f,  1.0f,  0.0f, 0.0f },
	{  1.0f, -1.0f,  1.0f,  1.0f, 0.0f },
	// Back face
	{ -1.0f,  1.0f, -1.0f,  1.0f, 1.0f },
	{  1.0f,  1.0f, -1.0f,  0.0f, 1.0f },
	{  1.0f, -1.0f, -1.0f,  0.0f, 0.0f },
	{ -1.0f, -1.0f, -1.0f,  1.0f, 0.0f },
	// Top face
	{  1.0f,  1.0f, -1.0f,  1.0f, 1.0f },
	{ -1.0f,  1.0f, -1.0f,  0.0f, 1.0f },
	{ -1.0f,  1.0f,  1.0f,  0.0f, 0.0f },
	{  1.0f,  1.0f,  1.0f,  1.0f, 0.0f },
	// Bottom face
	{  1.0f, -1.0f,  1.0f,  0.0f, 0.0f },
	{ -1.0f, -1.0f,  1.0f,  1.0f, 0.0f },
	{ -1.0f, -1.0f, -1.0f,  1.0f, 1.0f },
	{  1.0f, -1.0f, -1.0f,  0.0f, 1.0f },
	// Right face
	{  1.0f, -1.0f, -1.0f,  1.0f, 0.0f },
	{  1.0f,  1.0f, -1.0f,  1.0f, 1.0f },
	{  1.0f,  1.0f,  1.0f,  0.0f, 1.0f },
	{  1.0f, -1.0f,  1.0f,  0.0f, 0.0f },
	// Left face
	{ -1.0f, -1.0f, -1.0f,  0.0f, 0.0f },
	{ -1.0f, -1.0f,  1.0f,  1.0f, 0.0f },
	{ -1.0f,  1.0f,  1.0f,  1.0f, 1.0f },
	{ -1.0f,  1.0f, -1.0f,  0.0f, 1.0f }
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


static SDL_GPUGraphicsPipeline* pso = NULL;
static SDL_GPUBuffer* vtxBuffer = NULL;
static SDL_GPUBuffer* idxBuffer = NULL;
static SDL_GPUSampler* sampler = NULL;
static SDL_GPUTexture* texture = NULL;

static Mtx projection;

static float xRot = 0.0f, yRot = 0.0f, zRot = 0.0f;


static bool Lesson29_Init(NeHeContext* restrict ctx)
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
			.compare_op = SDL_GPU_COMPAREOP_LESS,
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

	if ((texture = BuildTexture(ctx)) == NULL)
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

	return true;
}

static void Lesson29_Quit(NeHeContext* restrict ctx)
{
	SDL_ReleaseGPUBuffer(ctx->device, idxBuffer);
	SDL_ReleaseGPUBuffer(ctx->device, vtxBuffer);
	SDL_ReleaseGPUSampler(ctx->device, sampler);
	SDL_ReleaseGPUTexture(ctx->device, texture);
	SDL_ReleaseGPUGraphicsPipeline(ctx->device, pso);
}

static void Lesson29_Resize(NeHeContext* restrict ctx, int width, int height)
{
	(void)ctx;

	// Avoid division by zero by clamping height
	height = SDL_max(height, 1);
	// Recalculate projection matrix
	projection = Mtx_Perspective(45.0f, (float)width / (float)height, 0.1f, 100.0f);
}

static void Lesson29_Draw(NeHeContext* restrict ctx, SDL_GPUCommandBuffer* restrict cmd,
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

	// Set-up model matrix
	Mtx model = Mtx_Translation(0.0f, 0.0f, -5.0f);
	Mtx_Rotate(&model, xRot, 1.0f, 0.0f, 0.0f);
	Mtx_Rotate(&model, yRot, 0.0f, 1.0f, 0.0f);
	Mtx_Rotate(&model, zRot, 0.0f, 0.0f, 1.0f);

	// Push shader uniforms
	Mtx modelViewProj = Mtx_Multiply(&projection, &model);
	SDL_PushGPUVertexUniformData(cmd, 0, &modelViewProj, sizeof(Mtx));

	// Draw textured cube
	SDL_DrawGPUIndexedPrimitives(pass, SDL_arraysize(indices), 1, 0, 0, 0);

	SDL_EndGPURenderPass(pass);

	xRot += 0.3f;
	yRot += 0.2f;
	zRot += 0.4f;
}


const struct AppConfig appConfig =
{
	.title = "Andreas L\u00F6ffler, Rob Fletcher & NeHe's Blitter & Raw Image Loading Tutorial",
	.width = 640, .height = 480,
	.createDepthFormat = SDL_GPU_TEXTUREFORMAT_D16_UNORM,
	.init = Lesson29_Init,
	.quit = Lesson29_Quit,
	.resize = Lesson29_Resize,
	.draw = Lesson29_Draw
};
