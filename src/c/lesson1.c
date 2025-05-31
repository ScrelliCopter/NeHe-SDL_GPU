/*
 * SPDX-FileCopyrightText: (C) 2025 a dinosaur
 * SPDX-License-Identifier: Zlib
 */

#include "nehe.h"


static void Lesson1_Draw(NeHeContext* restrict ctx, SDL_GPUCommandBuffer* restrict cmd, SDL_GPUTexture* restrict swapchain)
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
	SDL_EndGPURenderPass(pass);
}


const struct AppConfig appConfig =
{
	.title = "NeHe's OpenGL Framework",
	.width = 640, .height = 480,
	.init = NULL,
	.quit = NULL,
	.resize = NULL,
	.draw = Lesson1_Draw
};
