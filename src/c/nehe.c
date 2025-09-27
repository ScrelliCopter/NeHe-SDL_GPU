/*
 * SPDX-FileCopyrightText: (C) 2025 a dinosaur
 * SPDX-License-Identifier: Zlib
 */

#include "nehe.h"


static uint32_t rngState = 1;

int NeHe_Random(void)
{
	rngState = rngState * 214013 + 2531011;
	return (int)((rngState >> 16) & 0x7FFF);  // (s / 65536) % 32768
}

void NeHe_RandomSeed(uint32_t seed)
{
	rngState = seed;
}


bool NeHe_InitGPU(NeHeContext* ctx, const char* title, int width, int height)
{
	// Create window
	ctx->window = SDL_CreateWindow(title, width, height, SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY);
	if (!ctx->window)
	{
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "SDL_CreateWindow: %s", SDL_GetError());
		return false;
	}

	// Open GPU device
	const SDL_GPUShaderFormat formats =
		// FIXME: Re-enable D3D12 later when lesson9 works properly
		SDL_GPU_SHADERFORMAT_METALLIB | SDL_GPU_SHADERFORMAT_MSL |
		SDL_GPU_SHADERFORMAT_SPIRV | SDL_GPU_SHADERFORMAT_DXIL;
	ctx->device = SDL_CreateGPUDevice(formats, true, NULL);
	if (!ctx->device)
	{
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "SDL_CreateGPUDevice: %s", SDL_GetError());
		return false;
	}

	// Attach window to the GPU device
	if (!SDL_ClaimWindowForGPUDevice(ctx->device, ctx->window))
	{
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "SDL_ClaimWindowForGPUDevice: %s", SDL_GetError());
		return false;
	}

	// Enable VSync
	SDL_SetGPUSwapchainParameters(ctx->device, ctx->window,
		SDL_GPU_SWAPCHAINCOMPOSITION_SDR,
		SDL_GPU_PRESENTMODE_VSYNC);

	return true;
}

bool NeHe_SetupDepthTexture(NeHeContext* ctx, uint32_t width, uint32_t height,
	SDL_GPUTextureFormat format, float clearDepth)
{
	if (ctx->depthTexture)
	{
		SDL_ReleaseGPUTexture(ctx->device, ctx->depthTexture);
		ctx->depthTexture = NULL;
	}

	SDL_PropertiesID props = SDL_CreateProperties();
	if (props == 0)
	{
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "SDL_CreateProperties: %s", SDL_GetError());
		return false;
	}
	// Workaround for https://github.com/libsdl-org/SDL/issues/10758
	SDL_SetFloatProperty(props, SDL_PROP_GPU_TEXTURE_CREATE_D3D12_CLEAR_DEPTH_FLOAT, clearDepth);

	SDL_GPUTexture* texture = SDL_CreateGPUTexture(ctx->device, &(const SDL_GPUTextureCreateInfo)
	{
		.type = SDL_GPU_TEXTURETYPE_2D,
		.format = format,
		.width = width,
		.height = height,
		.layer_count_or_depth = 1,
		.num_levels = 1,
		.sample_count = SDL_GPU_SAMPLECOUNT_1,
		.usage = SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET,
		.props = props
	});
	SDL_DestroyProperties(props);
	if (!texture)
	{
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "SDL_CreateGPUTexture: %s", SDL_GetError());
		return false;
	}

	SDL_SetGPUTextureName(ctx->device, texture, "Depth Buffer Texture");
	ctx->depthTexture       = texture;
	ctx->depthTextureWidth  = width;
	ctx->depthTextureHeight = height;
	return true;
}

char* NeHe_ResourcePath(const NeHeContext* restrict ctx, const char* const restrict resourcePath)
{
	SDL_assert(ctx && ctx->baseDir && resourcePath);

	// Build path to resource: "{baseDir}/{resourcePath}"
	const size_t baseLen = SDL_strlen(ctx->baseDir);
	const size_t resourcePathLen = SDL_strlen(resourcePath);
	char* path = SDL_malloc(baseLen + resourcePathLen + 1);
	if (!path)
	{
		return NULL;
	}
	SDL_memcpy(path, ctx->baseDir, baseLen);
	SDL_memcpy(&path[baseLen], resourcePath, resourcePathLen);
	path[baseLen + resourcePathLen] = '\0';
	return path;
}

extern inline SDL_IOStream* NeHe_OpenResource(const NeHeContext* restrict ctx,
	const char* restrict resourcePath,
	const char* restrict mode);

SDL_GPUTexture* NeHe_LoadTexture(NeHeContext* restrict ctx, const char* const restrict resourcePath,
	bool flipVertical, bool genMipmaps)
{
	char* path = NeHe_ResourcePath(ctx, resourcePath);
	if (!path)
	{
		return NULL;
	}

	// Load image into a surface
	SDL_Surface* image = SDL_LoadBMP(path);
	SDL_free(path);
	if (!image)
	{
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "SDL_LoadBMP: %s", SDL_GetError());
		return NULL;
	}

	// Flip surface if requested
	if (flipVertical && !SDL_FlipSurface(image, SDL_FLIP_VERTICAL))
	{
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "SDL_FlipSurface: %s", SDL_GetError());
		SDL_DestroySurface(image);
		return NULL;
	}

	// Upload texture to GPU
	SDL_GPUTexture* texture = NeHe_CreateGPUTextureFromSurface(ctx, image, genMipmaps);
	SDL_DestroySurface(image);
	if (!texture)
	{
		return NULL;
	}

	return texture;
}

SDL_GPUTexture* NeHe_LoadTextureSeparateMask(NeHeContext* restrict ctx,
	const char* const restrict colorResourcePath, const char* const restrict maskResourcePath,
	bool flipVertical)
{
	char* colorPath = NeHe_ResourcePath(ctx, colorResourcePath);
	char* maskPath  = NeHe_ResourcePath(ctx, maskResourcePath);
	if (!colorPath || !maskPath)
	{
		SDL_free(maskPath);
		SDL_free(colorPath);
		return NULL;
	}

	// Load images to combine
	SDL_Surface* color = SDL_LoadBMP(colorPath);
	SDL_Surface* mask  = SDL_LoadBMP(maskPath);
	SDL_free(maskPath);
	SDL_free(colorPath);
	if (!color || !mask)
	{
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "SDL_LoadBMP: %s", SDL_GetError());
		SDL_DestroySurface(mask);
		SDL_DestroySurface(color);
		return NULL;
	}

	// Get mask format details
	const SDL_PixelFormatDetails* maskFmt = SDL_GetPixelFormatDetails(mask->format);
	if (!maskFmt)
	{
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "SDL_GetPixelFormatDetails: %s", SDL_GetError());
		SDL_DestroySurface(mask);
		SDL_DestroySurface(color);
		return NULL;
	}

	int maskValueOffset, maskValueStride;

	// The algorithm requires mask images with a byte-aligned 8-bit red channel
	if (maskFmt->Rmask != 0 && maskFmt->Rbits == 8 && (maskFmt->Rshift & 0x7) == 0 &&
		(maskFmt->bits_per_pixel >> 3) == maskFmt->bytes_per_pixel)
	{
		maskValueOffset = maskFmt->Rshift >> 3;
		maskValueStride = maskFmt->bytes_per_pixel;
	}
	else
	{
		// Convert the mask to something the algorithm works with
		SDL_Surface* newMask = SDL_ConvertSurface(mask, SDL_PIXELFORMAT_BGR24);
		SDL_DestroySurface(mask);
		if (!newMask)
		{
			SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "SDL_ConvertSurface: %s", SDL_GetError());
			SDL_DestroySurface(color);
			return NULL;
		}
		mask = newMask;
		maskValueOffset = 0;
		maskValueStride = 3;
	}

	// Create image from colour layer w/ alpha channel
	SDL_Surface* image = SDL_ConvertSurface(color, SDL_PIXELFORMAT_BGRA8888);
	SDL_DestroySurface(color);
	if (!image)
	{
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "SDL_ConvertSurface: %s", SDL_GetError());
		SDL_DestroySurface(mask);
		return NULL;
	}

	// Place an inverted copy of the mask's red channel in the image's alpha channel
	if (!SDL_LockSurface(image) || !SDL_LockSurface(mask))
	{
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "SDL_LockSurface: %s", SDL_GetError());
		SDL_DestroySurface(image);
		SDL_DestroySurface(mask);
	}
	Uint8* src = (Uint8*)mask->pixels;
	Uint8* dst = (Uint8*)image->pixels;
	const int width = SDL_min(image->w, mask->w);
	for (int height = SDL_min(image->h, mask->h); height; height--)
	{
		for (int x = 0; x < width; ++x)
		{
			dst[4 * x] = src[maskValueStride * x + maskValueOffset] ^ 0xFF;
		}
		src += mask->pitch;
		dst += image->pitch;
	}
	SDL_UnlockSurface(mask);
	SDL_UnlockSurface(image);

	SDL_DestroySurface(mask);  // We can now free the mask

	// Flip surface if requested
	if (flipVertical && !SDL_FlipSurface(image, SDL_FLIP_VERTICAL))
	{
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "SDL_FlipSurface: %s", SDL_GetError());
		SDL_DestroySurface(image);
		return NULL;
	}

	// Upload texture to GPU
	SDL_GPUTexture* texture = NeHe_CreateGPUTextureFromSurface(ctx, image, false);
	SDL_DestroySurface(image);
	if (!texture)
	{
		return NULL;
	}

	return texture;
}

SDL_GPUTexture* NeHe_CreateGPUTextureFromPixels(NeHeContext* restrict ctx, const void* restrict data,
	size_t dataSize, const SDL_GPUTextureCreateInfo* const restrict createInfo, bool genMipmaps)
{
	SDL_assert(dataSize <= UINT32_MAX);
	SDL_GPUDevice* device = ctx->device;

	SDL_GPUTexture* texture = SDL_CreateGPUTexture(device, createInfo);
	if (!texture)
	{
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "SDL_CreateGPUTexture: %s", SDL_GetError());
		return NULL;
	}

	// Create and copy image data to a transfer buffer
	SDL_GPUTransferBuffer* xferBuffer = SDL_CreateGPUTransferBuffer(device, &(const SDL_GPUTransferBufferCreateInfo)
	{
		.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
		.size = (Uint32)dataSize
	});
	if (!xferBuffer)
	{
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "SDL_CreateGPUTransferBuffer: %s", SDL_GetError());
		SDL_ReleaseGPUTexture(device, texture);
		return NULL;
	}

	void* map = SDL_MapGPUTransferBuffer(device, xferBuffer, false);
	if (!map)
	{
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "SDL_MapGPUTransferBuffer: %s", SDL_GetError());
		SDL_ReleaseGPUTransferBuffer(device, xferBuffer);
		SDL_ReleaseGPUTexture(device, texture);
		return NULL;
	}
	SDL_memcpy(map, data, dataSize);
	SDL_UnmapGPUTransferBuffer(device, xferBuffer);

	// Upload the transfer data to the GPU resources
	SDL_GPUCommandBuffer* cmd = SDL_AcquireGPUCommandBuffer(device);
	if (!cmd)
	{
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "SDL_AcquireGPUCommandBuffer: %s", SDL_GetError());
		SDL_ReleaseGPUTransferBuffer(device, xferBuffer);
		SDL_ReleaseGPUTexture(device, texture);
		return NULL;
	}

	SDL_GPUCopyPass* pass = SDL_BeginGPUCopyPass(cmd);
	SDL_UploadToGPUTexture(pass, &(const SDL_GPUTextureTransferInfo)
	{
		.transfer_buffer = xferBuffer,
		.offset = 0
	}, &(const SDL_GPUTextureRegion)
	{
		.texture = texture,
		.w = createInfo->width,
		.h = createInfo->height,
		.d = createInfo->layer_count_or_depth
	}, false);
	SDL_EndGPUCopyPass(pass);

	if (genMipmaps)
	{
		SDL_GenerateMipmapsForGPUTexture(cmd, texture);
	}

	SDL_SubmitGPUCommandBuffer(cmd);
	SDL_ReleaseGPUTransferBuffer(device, xferBuffer);
	return texture;
}

SDL_GPUTexture* NeHe_CreateGPUTextureFromSurface(NeHeContext* restrict ctx, const SDL_Surface* restrict surface,
	bool genMipmaps)
{
	SDL_GPUTextureCreateInfo info;
	SDL_zero(info);
	info.type   = SDL_GPU_TEXTURETYPE_2D;
	info.format = SDL_GPU_TEXTUREFORMAT_INVALID;
	info.usage  = SDL_GPU_TEXTUREUSAGE_SAMPLER;
	info.width  = (Uint32)surface->w;
	info.height = (Uint32)surface->h;
	info.layer_count_or_depth = 1;
	info.num_levels           = 1;

	bool needsConvert = false;
	switch (surface->format)
	{
	// FIXME: I'm not sure that these are endian-safe
	case SDL_PIXELFORMAT_RGBA32:        info.format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM; break;
	case SDL_PIXELFORMAT_RGBA64:        info.format = SDL_GPU_TEXTUREFORMAT_R16G16B16A16_UNORM; break;
	case SDL_PIXELFORMAT_RGB565:        info.format = SDL_GPU_TEXTUREFORMAT_B5G6R5_UNORM; break;
	case SDL_PIXELFORMAT_ARGB1555:      info.format = SDL_GPU_TEXTUREFORMAT_B5G5R5A1_UNORM; break;
	case SDL_PIXELFORMAT_BGRA4444:      info.format = SDL_GPU_TEXTUREFORMAT_B4G4R4A4_UNORM; break;
	case SDL_PIXELFORMAT_BGRA32:        info.format = SDL_GPU_TEXTUREFORMAT_B8G8R8A8_UNORM; break;
	case SDL_PIXELFORMAT_RGBA64_FLOAT:  info.format = SDL_GPU_TEXTUREFORMAT_R16G16B16A16_FLOAT; break;
	case SDL_PIXELFORMAT_RGBA128_FLOAT: info.format = SDL_GPU_TEXTUREFORMAT_R32G32B32A32_FLOAT; break;
	default:
		needsConvert = true;
		info.format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM;
		break;
	}

	size_t dataSize = (size_t)surface->w * (size_t)surface->h;
	void* data = NULL;
	SDL_Surface* conv = NULL;
	if (needsConvert)
	{
		// Convert pixel format if required
		if ((conv = SDL_ConvertSurface((SDL_Surface*)surface, SDL_PIXELFORMAT_ABGR8888)) == NULL)
		{
			SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "SDL_ConvertSurface: %s", SDL_GetError());
			SDL_free(data);
			return NULL;
		}
		dataSize *= SDL_BYTESPERPIXEL(conv->format);
		data = conv->pixels;
	}
	else
	{
		dataSize *= SDL_BYTESPERPIXEL(surface->format);
		data = surface->pixels;
	}

	if (genMipmaps)
	{
		info.usage |= SDL_GPU_TEXTUREUSAGE_COLOR_TARGET;
		// floor(log₂(max(𝑤,ℎ))) + 1
		info.num_levels = (Uint32)SDL_MostSignificantBitIndex32(SDL_max(info.width, info.height)) + 1;
	}

	SDL_GPUTexture* texture = NeHe_CreateGPUTextureFromPixels(ctx, data, dataSize, &info, genMipmaps);
	SDL_DestroySurface(conv);
	return texture;
}

static char* ReadBlob(const char* const restrict path, size_t* restrict outLength)
{
	SDL_ClearError();
	SDL_IOStream* file = SDL_IOFromFile(path, "rb");
	if (!file)
	{
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "SDL_IOFromFile: %s", SDL_GetError());
		return NULL;
	}

	// Allocate a buffer of the size of the file
	if (SDL_SeekIO(file, 0, SDL_IO_SEEK_END) == -1)
	{
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "SDL_SeekIO: %s", SDL_GetError());
	}
	const int64_t size = SDL_TellIO(file);
	char* data;
	if (size < 0 || (data = SDL_malloc((size_t)size)) == NULL)
	{
		SDL_CloseIO(file);
		return NULL;
	}
	if (SDL_SeekIO(file, 0, SDL_IO_SEEK_SET) != 0)
	{
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "SDL_SeekIO: %s", SDL_GetError());
	}

	SDL_ClearError();
	// Read the file contents into the buffer
	const size_t read = SDL_ReadIO(file, data, (size_t)size);
	if (read == 0 && SDL_GetIOStatus(file) == SDL_IO_STATUS_ERROR)
	{
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "SDL_ReadIO: %s", SDL_GetError());
	}
	if (!SDL_CloseIO(file))
	{
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "SDL_CloseIO: %s", SDL_GetError());
	}
	if (read != (size_t)size)
	{
		SDL_free(data);
		return NULL;
	}

	if (outLength)
	{
		*outLength = (size_t)size;
	}
	return data;
}

void* NeHe_ReadResourceBlob(const NeHeContext* restrict ctx,
	const char* const restrict resourcePath, size_t* restrict outLength)
{
	char* path = NeHe_ResourcePath(ctx, resourcePath);
	void* result = ReadBlob(path, outLength);
	SDL_free(path);
	return result;
}

static SDL_GPUShader* LoadShaderBlob(NeHeContext* restrict ctx,
	const char* restrict code, size_t codeLen,
	const NeHeShaderProgramCreateInfo* restrict info,
	SDL_GPUShaderFormat format, SDL_GPUShaderStage type,
	const char* const restrict main)
{
	if (!code)
	{
		return NULL;
	}

	SDL_GPUShader* shader = SDL_CreateGPUShader(ctx->device, &(const SDL_GPUShaderCreateInfo)
	{
		.code_size = codeLen,
		.code = (const Uint8*)code,
		.entrypoint = main,
		.format = format,
		.stage = type,
		.num_samplers = (type == SDL_GPU_SHADERSTAGE_FRAGMENT) ? info->fragmentSamplers : 0,
		.num_storage_buffers = (type == SDL_GPU_SHADERSTAGE_VERTEX) ? info->vertexStorage : 0,
		.num_uniform_buffers = (type == SDL_GPU_SHADERSTAGE_VERTEX) ? info->vertexUniforms : info->fragmentUniforms
	});
	if (!shader)
	{
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "SDL_CreateGPUShader: %s", SDL_GetError());
		return NULL;
	}
	return shader;
}

static SDL_GPUShader* LoadShader(NeHeContext* restrict ctx, const char* restrict path,
	const NeHeShaderProgramCreateInfo* restrict info, SDL_GPUShaderFormat format, SDL_GPUShaderStage type,
	const char* const restrict main)
{
	size_t size;
	char* data = ReadBlob(path, &size);
	SDL_GPUShader *shader = LoadShaderBlob(ctx, data, size, info, format, type, main);
	SDL_free(data);
	return shader;
}

bool NeHe_LoadShaders(NeHeContext* restrict ctx,
	SDL_GPUShader** restrict outVertex,
	SDL_GPUShader** restrict outFragment,
	const char* restrict name,
	const NeHeShaderProgramCreateInfo* restrict info)
{
	SDL_GPUShader *vtxShader = NULL, *frgShader = NULL;

	// Build path to shader: "{base}/Data/Shaders/{name}.{ext}"
	const char* resources = SDL_GetBasePath();  // Resources directory
	const size_t resourcesLen = SDL_strlen(resources);
	const size_t nameLen = SDL_strlen(name);
	const size_t basenameLen = resourcesLen + 13 + nameLen;
	char* path = SDL_malloc(basenameLen + 10);
	if (!path)
	{
		return false;
	}
	SDL_memcpy(path, resources, resourcesLen);
	SDL_memcpy(&path[resourcesLen], "Data", 4);
	path[resourcesLen + 4] = resources[resourcesLen - 1];  // Copy path separator
	SDL_memcpy(&path[resourcesLen + 5], "Shaders", 7);
	path[resourcesLen + 12] = resources[resourcesLen - 1];  // Copy path separator
	SDL_memcpy(&path[resourcesLen + 13], name, nameLen);

	const SDL_GPUShaderFormat availableFormats = SDL_GetGPUShaderFormats(ctx->device);
	if (availableFormats & (SDL_GPU_SHADERFORMAT_METALLIB | SDL_GPU_SHADERFORMAT_MSL))
	{
		size_t size;
		if (availableFormats & SDL_GPU_SHADERFORMAT_METALLIB)  // Apple Metal (compiled library)
		{
			const SDL_GPUShaderFormat format = SDL_GPU_SHADERFORMAT_METALLIB;
			SDL_memcpy(&path[basenameLen], ".metallib", 10);
			char* lib = ReadBlob(path, &size);
			vtxShader = LoadShaderBlob(ctx, lib, size, info, format, SDL_GPU_SHADERSTAGE_VERTEX, "VertexMain");
			frgShader = LoadShaderBlob(ctx, lib, size, info, format, SDL_GPU_SHADERSTAGE_FRAGMENT, "FragmentMain");
			SDL_free(lib);
		}
		if ((!vtxShader || !frgShader) && availableFormats & SDL_GPU_SHADERFORMAT_MSL)  // Apple Metal (source)
		{
			const SDL_GPUShaderFormat format = SDL_GPU_SHADERFORMAT_MSL;
			SDL_memcpy(&path[basenameLen], ".metal", 7);
			char* src = ReadBlob(path, &size);
			if (!vtxShader)
				vtxShader = LoadShaderBlob(ctx, src, size, info, format, SDL_GPU_SHADERSTAGE_VERTEX, "VertexMain");
			if (!frgShader)
				frgShader = LoadShaderBlob(ctx, src, size, info, format, SDL_GPU_SHADERSTAGE_FRAGMENT, "FragmentMain");
			SDL_free(src);
		}
	}
	else if (availableFormats & SDL_GPU_SHADERFORMAT_SPIRV)  // Vulkan
	{
		const SDL_GPUShaderFormat format = SDL_GPU_SHADERFORMAT_SPIRV;
		SDL_memcpy(&path[basenameLen], ".vtx.spv", 9);
		vtxShader = LoadShader(ctx, path, info, format, SDL_GPU_SHADERSTAGE_VERTEX, "VertexMain");
		SDL_memcpy(&path[basenameLen], ".frg.spv", 9);
		frgShader = LoadShader(ctx, path, info, format, SDL_GPU_SHADERSTAGE_FRAGMENT, "FragmentMain");
	}
	else if (availableFormats & SDL_GPU_SHADERFORMAT_DXIL)  // Direct3D 12 Shader Model 6.0
	{
		const SDL_GPUShaderFormat format = SDL_GPU_SHADERFORMAT_DXIL;
		SDL_memcpy(&path[basenameLen], ".vtx.dxb", 9);
		vtxShader = LoadShader(ctx, path, info, format, SDL_GPU_SHADERSTAGE_VERTEX, "VertexMain");
		SDL_memcpy(&path[basenameLen], ".pxl.dxb", 9);
		frgShader = LoadShader(ctx, path, info, format, SDL_GPU_SHADERSTAGE_FRAGMENT, "PixelMain");
	}
	else if (availableFormats & SDL_GPU_SHADERFORMAT_DXBC)  // Direct3D 12 Shader Model 5.1
	{
		const SDL_GPUShaderFormat format = SDL_GPU_SHADERFORMAT_DXBC;
		SDL_memcpy(&path[basenameLen], ".vtx.fxb", 9);
		vtxShader = LoadShader(ctx, path, info, format, SDL_GPU_SHADERSTAGE_VERTEX, "VertexMain");
		SDL_memcpy(&path[basenameLen], ".pxl.fxb", 9);
		frgShader = LoadShader(ctx, path, info, format, SDL_GPU_SHADERSTAGE_FRAGMENT, "PixelMain");
	}

	SDL_free(path);
	if (!vtxShader || !frgShader)
	{
		if (vtxShader)
			SDL_ReleaseGPUShader(ctx->device, vtxShader);
		if (frgShader)
			SDL_ReleaseGPUShader(ctx->device, frgShader);
		return false;
	}

	*outVertex = vtxShader;
	*outFragment = frgShader;
	return true;
}

SDL_GPUBuffer* NeHe_CreateBuffer(NeHeContext* restrict ctx, const void* restrict data, uint32_t size,
	SDL_GPUBufferUsageFlags usage)
{
	// Create GPU data buffer
	SDL_GPUBuffer* buffer = SDL_CreateGPUBuffer(ctx->device, &(const SDL_GPUBufferCreateInfo)
	{
		.usage = usage,
		.size = size
	});
	if (!buffer)
	{
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "SDL_CreateGPUBuffer: %s", SDL_GetError());
		return false;
	}

	// Create data transfer buffer
	SDL_GPUTransferBuffer* xferBuffer = SDL_CreateGPUTransferBuffer(ctx->device, &(const SDL_GPUTransferBufferCreateInfo)
	{
		.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
		.size = size
	});
	if (!xferBuffer)
	{
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "SDL_CreateGPUTransferBuffer: %s", SDL_GetError());
		SDL_ReleaseGPUBuffer(ctx->device, buffer);
		return false;
	}

	// Map transfer buffer and copy the payload data
	Uint8* map = SDL_MapGPUTransferBuffer(ctx->device, xferBuffer, false);
	if (!map)
	{
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "SDL_MapGPUTransferBuffer: %s", SDL_GetError());
		SDL_ReleaseGPUTransferBuffer(ctx->device, xferBuffer);
		SDL_ReleaseGPUBuffer(ctx->device, buffer);
		return false;
	}
	SDL_memcpy(map, data, (size_t)size);
	SDL_UnmapGPUTransferBuffer(ctx->device, xferBuffer);

	SDL_GPUCommandBuffer* cmd = SDL_AcquireGPUCommandBuffer(ctx->device);
	if (!cmd)
	{
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "SDL_AcquireGPUCommandBuffer: %s", SDL_GetError());
		SDL_ReleaseGPUTransferBuffer(ctx->device, xferBuffer);
		SDL_ReleaseGPUBuffer(ctx->device, buffer);
		return false;
	}

	// Upload the data into the GPU buffer
	SDL_GPUCopyPass* pass = SDL_BeginGPUCopyPass(cmd);
	SDL_UploadToGPUBuffer(pass, &(const SDL_GPUTransferBufferLocation)
	{
		.transfer_buffer = xferBuffer,
		.offset = 0
	}, &(const SDL_GPUBufferRegion)
	{
		.buffer = buffer,
		.offset = 0,
		.size = size
	}, false);
	SDL_EndGPUCopyPass(pass);
	SDL_SubmitGPUCommandBuffer(cmd);

	SDL_ReleaseGPUTransferBuffer(ctx->device, xferBuffer);

	return buffer;
}

bool NeHe_CreateVertexIndexBuffer(NeHeContext* restrict ctx,
	SDL_GPUBuffer** restrict outVertexBuffer,
	SDL_GPUBuffer** restrict outIndexBuffer,
	const void* restrict vertices, uint32_t verticesSize,
	const void* restrict indices, uint32_t indicesSize)
{
	// Create vertex data buffer
	SDL_GPUBuffer* vtxBuffer = SDL_CreateGPUBuffer(ctx->device, &(const SDL_GPUBufferCreateInfo)
	{
		.usage = SDL_GPU_BUFFERUSAGE_VERTEX,
		.size = verticesSize
	});
	if (!vtxBuffer)
	{
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "SDL_CreateGPUBuffer: %s", SDL_GetError());
		return false;
	}

	// Create index data buffer
	SDL_GPUBuffer* idxBuffer = SDL_CreateGPUBuffer(ctx->device, &(const SDL_GPUBufferCreateInfo)
	{
		.usage = SDL_GPU_BUFFERUSAGE_INDEX,
		.size = indicesSize
	});
	if (!idxBuffer)
	{
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "SDL_CreateGPUBuffer: %s", SDL_GetError());
		SDL_ReleaseGPUBuffer(ctx->device, vtxBuffer);
		return false;
	}

	// Create vertex transfer buffer
	SDL_GPUTransferBuffer* vtxXferBuffer = SDL_CreateGPUTransferBuffer(ctx->device, &(const SDL_GPUTransferBufferCreateInfo)
	{
		.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
		.size = verticesSize
	});
	if (!vtxXferBuffer)
	{
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "SDL_CreateGPUTransferBuffer: %s", SDL_GetError());
		SDL_ReleaseGPUBuffer(ctx->device, idxBuffer);
		SDL_ReleaseGPUBuffer(ctx->device, vtxBuffer);
		return false;
	}

	// Create index transfer buffer
	SDL_GPUTransferBuffer* idxXferBuffer = SDL_CreateGPUTransferBuffer(ctx->device, &(const SDL_GPUTransferBufferCreateInfo)
	{
		.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
		.size = indicesSize
	});
	if (!idxXferBuffer)
	{
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "SDL_CreateGPUTransferBuffer: %s", SDL_GetError());
		SDL_ReleaseGPUTransferBuffer(ctx->device, vtxXferBuffer);
		SDL_ReleaseGPUBuffer(ctx->device, idxBuffer);
		SDL_ReleaseGPUBuffer(ctx->device, vtxBuffer);
		return false;
	}

	// Map transfer buffer and copy the vertex data
	Uint8* map = SDL_MapGPUTransferBuffer(ctx->device, vtxXferBuffer, false);
	if (!map)
	{
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "SDL_MapGPUTransferBuffer: %s", SDL_GetError());
		SDL_ReleaseGPUTransferBuffer(ctx->device, idxXferBuffer);
		SDL_ReleaseGPUTransferBuffer(ctx->device, vtxXferBuffer);
		SDL_ReleaseGPUBuffer(ctx->device, idxBuffer);
		SDL_ReleaseGPUBuffer(ctx->device, vtxBuffer);
		return false;
	}
	SDL_memcpy(map, vertices, (size_t)verticesSize);
	SDL_UnmapGPUTransferBuffer(ctx->device, vtxXferBuffer);

	// Map transfer buffer and copy the index data
	map = SDL_MapGPUTransferBuffer(ctx->device, idxXferBuffer, false);
	if (!map)
	{
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "SDL_MapGPUTransferBuffer: %s", SDL_GetError());
		SDL_ReleaseGPUTransferBuffer(ctx->device, idxXferBuffer);
		SDL_ReleaseGPUTransferBuffer(ctx->device, vtxXferBuffer);
		SDL_ReleaseGPUBuffer(ctx->device, idxBuffer);
		SDL_ReleaseGPUBuffer(ctx->device, vtxBuffer);
		return false;
	}
	SDL_memcpy(map, indices, (size_t)indicesSize);
	SDL_UnmapGPUTransferBuffer(ctx->device, idxXferBuffer);

	SDL_GPUCommandBuffer* cmd = SDL_AcquireGPUCommandBuffer(ctx->device);
	if (!cmd)
	{
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "SDL_AcquireGPUCommandBuffer: %s", SDL_GetError());
		SDL_ReleaseGPUTransferBuffer(ctx->device, idxXferBuffer);
		SDL_ReleaseGPUTransferBuffer(ctx->device, vtxXferBuffer);
		SDL_ReleaseGPUBuffer(ctx->device, idxBuffer);
		SDL_ReleaseGPUBuffer(ctx->device, vtxBuffer);
		return false;
	}

	// Upload the vertex & index data into the GPU buffer(s)
	SDL_GPUCopyPass* pass = SDL_BeginGPUCopyPass(cmd);
	SDL_UploadToGPUBuffer(pass, &(const SDL_GPUTransferBufferLocation)
	{
		.transfer_buffer = vtxXferBuffer,
		.offset = 0
	}, &(const SDL_GPUBufferRegion)
	{
		.buffer = vtxBuffer,
		.offset = 0,
		.size = verticesSize
	}, false);
	SDL_UploadToGPUBuffer(pass, &(const SDL_GPUTransferBufferLocation)
	{
		.transfer_buffer = idxXferBuffer,
		.offset = 0
	}, &(const SDL_GPUBufferRegion)
	{
		.buffer = idxBuffer,
		.offset = 0,
		.size = indicesSize
	}, false);
	SDL_EndGPUCopyPass(pass);
	SDL_SubmitGPUCommandBuffer(cmd);

	SDL_ReleaseGPUTransferBuffer(ctx->device, idxXferBuffer);
	SDL_ReleaseGPUTransferBuffer(ctx->device, vtxXferBuffer);

	*outVertexBuffer = vtxBuffer;
	*outIndexBuffer = idxBuffer;
	return true;
}
