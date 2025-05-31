/*
 * SPDX-FileCopyrightText: (C) 2025 a dinosaur
 * SPDX-License-Identifier: Zlib
 */

#include "nehe.h"
#define SDL_MAIN_USE_CALLBACKS
#include <SDL3/SDL_main.h>


typedef struct
{
	NeHeContext ctx;
	bool fullscreen;
} AppState;

SDL_AppResult SDLCALL SDL_AppInit(void** appstate, int argc, char* argv[])
{
	(void)argc; (void)argv;

	// Initialise SDL
	if (!SDL_Init(SDL_INIT_VIDEO))
	{
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "SDL_Init: %s", SDL_GetError());
		return SDL_APP_FAILURE;
	}

	SDL_SetCurrentThreadPriority(SDL_THREAD_PRIORITY_HIGH);

	// Allocate application context
	AppState* s = *appstate = SDL_malloc(sizeof(AppState));
	if (!s)
	{
		return SDL_APP_FAILURE;
	}
	*s = (AppState)
	{
		.ctx = (NeHeContext)
		{
			.window = NULL,
			.device = NULL
		},
		.fullscreen = false
	};
	NeHeContext* ctx = &s->ctx;
	ctx->baseDir = SDL_GetBasePath();  // Resources directory

	// Initialise GPU context
	if (!NeHe_InitGPU(ctx, appConfig.title, appConfig.width, appConfig.height))
	{
		return SDL_APP_FAILURE;
	}

	// Handle depth buffer texture creation if requested
	if (appConfig.createDepthFormat != SDL_GPU_TEXTUREFORMAT_INVALID)
	{
		unsigned backbufWidth, backbufHeight;
		SDL_GetWindowSizeInPixels(ctx->window, (int*)&backbufWidth, (int*)&backbufHeight);
		if (!NeHe_SetupDepthTexture(ctx, backbufWidth, backbufHeight, appConfig.createDepthFormat, 1.0f))
		{
			return false;
		}
	}

	if (appConfig.init)
	{
		if (!appConfig.init(ctx))
		{
			return SDL_APP_FAILURE;
		}
	}

	return SDL_APP_CONTINUE;
}

SDL_AppResult SDLCALL SDL_AppIterate(void* appstate)
{
	NeHeContext* ctx = &((AppState*)appstate)->ctx;
	SDL_GPUCommandBuffer* cmdbuf = SDL_AcquireGPUCommandBuffer(ctx->device);
	if (!cmdbuf)
	{
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "SDL_AcquireGPUCommandBuffer: %s", SDL_GetError());
		return SDL_APP_FAILURE;
	}

	SDL_GPUTexture* swapchainTex = NULL;
	uint32_t swapchainWidth, swapchainHeight;
	if (!SDL_WaitAndAcquireGPUSwapchainTexture(cmdbuf, ctx->window, &swapchainTex, &swapchainWidth, &swapchainHeight))
	{
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "SDL_WaitAndAcquireGPUSwapchainTexture: %s", SDL_GetError());
		SDL_CancelGPUCommandBuffer(cmdbuf);
		return SDL_APP_FAILURE;
	}
	if (!swapchainTex)
	{
		SDL_CancelGPUCommandBuffer(cmdbuf);
		return SDL_APP_CONTINUE;
	}

	if (appConfig.createDepthFormat != SDL_GPU_TEXTUREFORMAT_INVALID && ctx->depthTexture
		&& (ctx->depthTextureWidth != swapchainWidth || ctx->depthTextureHeight != swapchainHeight))
	{
		if (!NeHe_SetupDepthTexture(ctx, swapchainWidth, swapchainHeight, appConfig.createDepthFormat, 1.0f))
		{
			SDL_CancelGPUCommandBuffer(cmdbuf);
			return SDL_APP_FAILURE;
		}
	}

	if (appConfig.draw)
	{
		appConfig.draw(ctx, cmdbuf, swapchainTex);
	}

	SDL_SubmitGPUCommandBuffer(cmdbuf);
	return SDL_APP_CONTINUE;
}

SDL_AppResult SDLCALL SDL_AppEvent(void* appstate, SDL_Event* event)
{
	AppState* s = appstate;

	switch (event->type)
	{
	case SDL_EVENT_QUIT:
		return SDL_APP_SUCCESS;
	case SDL_EVENT_WINDOW_ENTER_FULLSCREEN:
	case SDL_EVENT_WINDOW_LEAVE_FULLSCREEN:
		s->fullscreen = event->type == SDL_EVENT_WINDOW_ENTER_FULLSCREEN;
		return SDL_APP_CONTINUE;
	case SDL_EVENT_KEY_DOWN:
		if (event->key.key == SDLK_ESCAPE)
		{
			return SDL_APP_SUCCESS;
		}
		if (event->key.key == SDLK_F1)
		{
			SDL_SetWindowFullscreen(s->ctx.window, !s->fullscreen);
			return SDL_APP_CONTINUE;
		}
		// Fallthrough
	case SDL_EVENT_KEY_UP:
		if (appConfig.key)
		{
			appConfig.key(&s->ctx, event->key.key, event->key.down, event->key.repeat);
		}
		return SDL_APP_CONTINUE;
	case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED:
		if (appConfig.resize)
		{
			appConfig.resize(&s->ctx, event->window.data1, event->window.data2);
		}
		return SDL_APP_CONTINUE;
	default:
		return SDL_APP_CONTINUE;
	}
}

void SDLCALL SDL_AppQuit(void* appstate, SDL_AppResult result)
{
	(void)result;

	if (appstate)
	{
		NeHeContext* ctx = &((AppState*)appstate)->ctx;
		if (appConfig.quit)
		{
			appConfig.quit(ctx);
		}
		if (appConfig.createDepthFormat != SDL_GPU_TEXTUREFORMAT_INVALID && ctx->depthTexture)
		{
			SDL_ReleaseGPUTexture(ctx->device, ctx->depthTexture);
		}
		SDL_ReleaseWindowFromGPUDevice(ctx->device, ctx->window);
		SDL_DestroyGPUDevice(ctx->device);
		SDL_DestroyWindow(ctx->window);
		SDL_free(ctx);
	}
	SDL_Quit();
}


#ifndef SDL_MAIN_USE_CALLBACKS
int main(int argc, char* argv[])
{
	void* appstate = NULL;
	SDL_AppResult res;
	if ((res = SDL_AppInit(&appstate, argc, argv)) != SDL_APP_CONTINUE)
	{
		goto Quit;
	}
	while (true)
	{
		SDL_Event event;
		while (SDL_PollEvent(&event))
		{
			if ((res = SDL_AppEvent(appstate, &event)) != SDL_APP_CONTINUE)
			{
				goto Quit;
			}
		}
		if ((res = SDL_AppIterate(appstate)) != SDL_APP_CONTINUE)
		{
			goto Quit;
		}
	}
Quit:
	SDL_AppQuit(appstate, res);
	return res == SDL_APP_SUCCESS ? 0 : 1;
}
#endif
