#ifndef APPLICATION_H
#define APPLICATION_H

#include <SDL3/SDL_gpu.h>
#include <SDL3/SDL_keycode.h>
#include <stdbool.h>

struct NeHeContext;

extern const struct AppConfig
{
	const char* const title;
	int width, height;
	SDL_GPUTextureFormat createDepthFormat;
	bool (*init)(struct NeHeContext*);
	void (*quit)(struct NeHeContext*);
	void (*resize)(struct NeHeContext*, int width, int height);
	void (*draw)(struct NeHeContext* restrict, SDL_GPUCommandBuffer* restrict, SDL_GPUTexture* restrict);
	void (*key)(struct NeHeContext*, SDL_Keycode, bool down, bool repeat);
} appConfig;

#endif//APPLICATION_H
