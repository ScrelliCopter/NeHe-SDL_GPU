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
	bool screenShot;
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
		.fullscreen = false,
		.screenShot = false
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
	AppState* s = (AppState*)appstate;
	SDL_GPUCommandBuffer* cmdbuf = SDL_AcquireGPUCommandBuffer(s->ctx.device);
	if (!cmdbuf)
	{
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "SDL_AcquireGPUCommandBuffer: %s", SDL_GetError());
		return SDL_APP_FAILURE;
	}

	SDL_GPUTexture* swapchainTex = NULL;
	uint32_t swapchainWidth, swapchainHeight;
	if (!SDL_WaitAndAcquireGPUSwapchainTexture(cmdbuf, s->ctx.window, &swapchainTex, &swapchainWidth, &swapchainHeight))
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

	if (appConfig.createDepthFormat != SDL_GPU_TEXTUREFORMAT_INVALID && s->ctx.depthTexture
		&& (s->ctx.depthTextureWidth != swapchainWidth || s->ctx.depthTextureHeight != swapchainHeight))
	{
		if (!NeHe_SetupDepthTexture(&s->ctx, swapchainWidth, swapchainHeight, appConfig.createDepthFormat, 1.0f))
		{
			SDL_CancelGPUCommandBuffer(cmdbuf);
			return SDL_APP_FAILURE;
		}
	}

	SDL_GPUTexture* screenshotTex = NULL;
	const SDL_GPUTextureFormat swapchainFormat = SDL_GetGPUSwapchainTextureFormat(s->ctx.device, s->ctx.window);
	if (s->screenShot)
	{
		s->screenShot = false;

		// Since the swapchain texture is write-only we need to render into a readable buffer
		screenshotTex = SDL_CreateGPUTexture(s->ctx.device, &(const SDL_GPUTextureCreateInfo)
		{
			.format = swapchainFormat,
			.usage = SDL_GPU_TEXTUREUSAGE_COLOR_TARGET | SDL_GPU_TEXTUREUSAGE_SAMPLER,
			.width = swapchainWidth,
			.height = swapchainHeight,
			.layer_count_or_depth = 1,
			.num_levels = 1
		});
		if (!screenshotTex)
		{
			SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "SDL_CreateGPUTexture: %s", SDL_GetError());
		}
	}

	if (appConfig.draw)
	{
		SDL_GPUTexture* backBuffer = screenshotTex ? screenshotTex : swapchainTex;
		appConfig.draw(&s->ctx, cmdbuf, backBuffer, (unsigned)swapchainWidth, (unsigned)swapchainHeight);
	}

	SDL_GPUTransferBuffer* screenshotXferBuffer = NULL;
	if (screenshotTex)
	{
		// Create screenshot transfer buffer
		screenshotXferBuffer = SDL_CreateGPUTransferBuffer(s->ctx.device, &(const SDL_GPUTransferBufferCreateInfo)
		{
			.usage = SDL_GPU_TRANSFERBUFFERUSAGE_DOWNLOAD,
			.size = 4 * swapchainWidth * swapchainHeight
		});
		if (!screenshotXferBuffer)
		{
			SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "SDL_CreateGPUTransferBuffer: %s", SDL_GetError());
		}

		SDL_GPUCopyPass* copyPass = SDL_BeginGPUCopyPass(cmdbuf);

		// Present the contents of the screenshot buffer
		SDL_CopyGPUTextureToTexture(copyPass,
			&(const SDL_GPUTextureLocation){ .texture = screenshotTex },
			&(const SDL_GPUTextureLocation){ .texture = swapchainTex },
			swapchainWidth, swapchainHeight, 1, false);

		if (screenshotXferBuffer)
		{
			// Copy the screenshot buffer into the transfer buffer
			SDL_DownloadFromGPUTexture(copyPass, &(const SDL_GPUTextureRegion)
			{
				.texture = screenshotTex,
				.w = swapchainWidth,
				.h = swapchainHeight,
				.d = 1
			}, &(const SDL_GPUTextureTransferInfo)
			{
				.transfer_buffer = screenshotXferBuffer
			});
		}

		// Wait for render & copy to complete
		SDL_EndGPUCopyPass(copyPass);
		SDL_GPUFence* fence = SDL_SubmitGPUCommandBufferAndAcquireFence(cmdbuf);
		SDL_WaitForGPUFences(s->ctx.device, true, &fence, 1);
		SDL_ReleaseGPUFence(s->ctx.device, fence);
		SDL_ReleaseGPUTexture(s->ctx.device, screenshotTex);
	}
	else
	{
		SDL_SubmitGPUCommandBuffer(cmdbuf);
	}

	if (screenshotXferBuffer)
	{
		NeHe_SaveBMPScreenshot(&s->ctx, appConfig.title, screenshotXferBuffer,
			swapchainFormat, (int)swapchainWidth, (int)swapchainHeight);

		// Destroy the transfer buffer
		SDL_UnmapGPUTransferBuffer(s->ctx.device, screenshotXferBuffer);
		SDL_ReleaseGPUTransferBuffer(s->ctx.device, screenshotXferBuffer);
	}

	return SDL_APP_CONTINUE;
}

SDL_AppResult SDLCALL SDL_AppEvent(void* appstate, SDL_Event* event)
{
	AppState* s = (AppState*)appstate;

	switch (event->type)
	{
	case SDL_EVENT_QUIT:
		return SDL_APP_SUCCESS;
	case SDL_EVENT_WINDOW_ENTER_FULLSCREEN:
	case SDL_EVENT_WINDOW_LEAVE_FULLSCREEN:
		s->fullscreen = event->type == SDL_EVENT_WINDOW_ENTER_FULLSCREEN;
		return SDL_APP_CONTINUE;
	case SDL_EVENT_KEY_DOWN:
		switch (event->key.key)
		{
		case SDLK_ESCAPE:
			return SDL_APP_SUCCESS;
		case SDLK_F1:
			SDL_SetWindowFullscreen(s->ctx.window, !s->fullscreen);
			return SDL_APP_CONTINUE;
		case SDLK_F12:
			s->screenShot = true;
			return SDL_APP_CONTINUE;
		default:
			break;
		}
		SDL_FALLTHROUGH;
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
