#ifndef NEHE_H
#define NEHE_H

#include "application.h"
#include "matrix.h"

#include <SDL3/SDL.h>
#include <stdint.h>
#include <stddef.h>
#include <float.h>

#if (__GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 6)) && __STDC_VERSION__ == 199901L
# define NEHE_STATIC_ASSERT(NAME, CONDITION) \
	_Pragma("GCC diagnostic ignored \"-Wpedantic\"") \
	_Static_assert(CONDITION)
#else
# define NEHE_STATIC_ASSERT(NAME, CONDITION) SDL_COMPILE_TIME_ASSERT(NAME, CONDITION)
#endif

typedef struct NeHeContext
{
	SDL_Window* window;
	SDL_GPUDevice* device;
	SDL_GPUTexture* depthTexture;
	uint32_t depthTextureWidth, depthTextureHeight;

	const char* baseDir;

} NeHeContext;

typedef struct
{
	unsigned vertexUniforms;
	unsigned vertexStorage;
	unsigned fragmentSamplers;
	unsigned fragmentUniforms;
} NeHeShaderProgramCreateInfo;

int NeHe_Random(void);
void NeHe_RandomSeed(uint32_t seed);

bool NeHe_InitGPU(NeHeContext* ctx, const char* title, int width, int height);
bool NeHe_SetupDepthTexture(NeHeContext* ctx, uint32_t width, uint32_t height,
	SDL_GPUTextureFormat format, float clearDepth);
char* NeHe_ResourcePath(const NeHeContext* restrict ctx, const char* restrict resourcePath);
inline SDL_IOStream* NeHe_OpenResource(const NeHeContext* restrict ctx,
	const char* const restrict resourcePath,
	const char* const restrict mode)
{
	char* path = NeHe_ResourcePath(ctx, resourcePath);
	if (!path)
	{
		return NULL;
	}
	SDL_IOStream* file = SDL_IOFromFile(path, mode);
	SDL_free(path);
	return file;
}
void* NeHe_ReadResourceBlob(const NeHeContext* restrict ctx,
	const char* const restrict resourcePath,
	size_t* restrict outLength);
SDL_GPUTexture* NeHe_LoadTexture(NeHeContext* restrict ctx, const char* restrict resourcePath,
	bool flipVertical, bool genMipmaps);
SDL_GPUTexture* NeHe_CreateGPUTextureFromSurface(NeHeContext* restrict ctx,
	const SDL_Surface* restrict surface, bool genMipmaps);
SDL_GPUTexture* NeHe_CreateGPUTextureFromPixels(NeHeContext* restrict ctx, const void* restrict data,
	size_t dataSize, const SDL_GPUTextureCreateInfo* restrict createInfo, bool genMipmaps);
bool NeHe_LoadShaders(NeHeContext* restrict ctx,
	SDL_GPUShader** restrict outVertex,
	SDL_GPUShader** restrict outFragment,
	const char* restrict name,
	const NeHeShaderProgramCreateInfo* restrict info);
SDL_GPUBuffer* NeHe_CreateBuffer(NeHeContext* restrict ctx, const void* restrict data, uint32_t size,
	SDL_GPUBufferUsageFlags usage);
bool NeHe_CreateVertexIndexBuffer(NeHeContext* restrict ctx,
	SDL_GPUBuffer** restrict outVertexBuffer,
	SDL_GPUBuffer** restrict outIndexBuffer,
	const void* restrict vertices, uint32_t verticesSize,
	const void* restrict indices, uint32_t indicesSize);

#endif//NEHE_H
