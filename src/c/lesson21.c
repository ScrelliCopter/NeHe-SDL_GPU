/*
 * SPDX-FileCopyrightText: (C) 2025 a dinosaur
 * SPDX-License-Identifier: Zlib
 */

#include "nehe.h"
#include "sound.h"


static NeHeSound* sndComplete = NULL, * sndDie = NULL, * sndFreeze = NULL, * sndHourglass = NULL;


static bool Lesson21_Init(NeHeContext* restrict ctx)
{
	sndComplete  = NeHe_LoadSound(ctx, "Data/Complete.wav");
	sndDie       = NeHe_LoadSound(ctx, "Data/Die.wav");
	sndFreeze    = NeHe_LoadSound(ctx, "Data/freeze.wav");
	sndHourglass = NeHe_LoadSound(ctx, "Data/hourglass.wav");
	NeHe_OpenSound();

	return true;
}

static void Lesson21_Quit(NeHeContext* restrict ctx)
{
	(void)ctx;

	NeHe_CloseSound();
	SDL_free(sndHourglass);
	SDL_free(sndFreeze);
	SDL_free(sndDie);
	SDL_free(sndComplete);
}

static void Lesson21_Draw(NeHeContext* restrict ctx, SDL_GPUCommandBuffer* restrict cmd,
	SDL_GPUTexture* restrict swapchain, unsigned swapchainW, unsigned swapchainH)
{
	(void)swapchainW; (void)swapchainH;

	const SDL_GPUColorTargetInfo colorInfo =
	{
		.texture = swapchain,
		.clear_color = { 0.0f, 0.0f, 0.0f, 0.5f },
		.load_op = SDL_GPU_LOADOP_CLEAR,
		.store_op = SDL_GPU_STOREOP_STORE
	};

	SDL_GPURenderPass* pass = SDL_BeginGPURenderPass(cmd, &colorInfo, 1, NULL);
	SDL_EndGPURenderPass(pass);
}

static void Lesson21_Key(NeHeContext* ctx, SDL_Keycode key, bool down, bool repeat)
{
	(void)ctx;

	if (down && !repeat)
	{
		switch (key)
		{
		case SDLK_1:
			NeHe_PlaySound(sndDie, NEHE_SND_SYNC);
			break;
		case SDLK_2:
			NeHe_PlaySound(sndComplete, NEHE_SND_SYNC);
			break;
		case SDLK_3:
			NeHe_PlaySound(sndFreeze, NEHE_SND_ASYNC | NEHE_SND_LOOP);
			break;
		case SDLK_4:
			NeHe_PlaySound(sndHourglass, NEHE_SND_ASYNC);
			break;
		case SDLK_5:
			NeHe_PlaySound(NULL, 0);
			break;
		default:
			break;
		}
	}
}


const struct AppConfig appConfig =
{
	.title = "NeHe's Line Tutorial",
	.width = 640, .height = 480,
	.init = Lesson21_Init,
	.quit = Lesson21_Quit,
	.resize = NULL,
	.draw = Lesson21_Draw,
	.key = Lesson21_Key
};
