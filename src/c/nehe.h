#ifndef NEHE_H
#define NEHE_H

#include "application.h"
#include "matrix.h"

#include <SDL3/SDL.h>
#include <stdint.h>
#include <stddef.h>

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
SDL_GPUTexture* NeHe_LoadTexture(NeHeContext* restrict ctx, const char* restrict resourcePath,
	bool flipVertical, bool genMipmaps);
SDL_GPUTexture* NeHe_CreateGPUTextureFromSurface(NeHeContext* restrict ctx, const SDL_Surface* restrict surface,
	bool genMipmaps);
bool NeHe_LoadShaders(NeHeContext* restrict ctx,
	SDL_GPUShader** restrict outVertex,
	SDL_GPUShader** restrict outFragment,
	const char* restrict name,
	const NeHeShaderProgramCreateInfo* restrict info);
SDL_GPUBuffer* NeHe_CreateVertexBuffer(NeHeContext* restrict ctx, const void* restrict vertices, uint32_t verticesSize);
bool NeHe_CreateVertexIndexBuffer(NeHeContext* restrict ctx,
	SDL_GPUBuffer** restrict outVertexBuffer,
	SDL_GPUBuffer** restrict outIndexBuffer,
	const void* restrict vertices, uint32_t verticesSize,
	const void* restrict indices, uint32_t indicesSize);

#endif//NEHE_H
