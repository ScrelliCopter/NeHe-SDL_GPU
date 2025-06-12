/*
 * SPDX-FileCopyrightText: (C) 2025 a dinosaur
 * SPDX-License-Identifier: Zlib
 */

pub mod config;

use crate::context::NeHeContext;
use crate::application::config::AppImplementation;
use crate::error::NeHeError;
use sdl3_sys::events::*;
use sdl3_sys::gpu::*;
use sdl3_sys::init::{SDL_Init, SDL_Quit, SDL_INIT_VIDEO};
use sdl3_sys::keycode::{SDLK_ESCAPE, SDLK_F1};
use std::mem::MaybeUninit;
use std::ptr::addr_of_mut;
use sdl3_sys::video::{SDL_GetWindowSizeInPixels, SDL_SetWindowFullscreen};

pub fn run<App: AppImplementation + Default + 'static>() -> Result<(), NeHeError>
{
	// Initialise SDL
	if !unsafe { SDL_Init(SDL_INIT_VIDEO) }
	{
		return Err(NeHeError::sdl("SDL_Init"))
	}

	// Initialise GPU context
	let mut ctx = NeHeContext::init(App::TITLE, App::WIDTH, App::HEIGHT)?;

	// Handle depth buffer texture creation if requested
	if App::CREATE_DEPTH_BUFFER != SDL_GPU_TEXTUREFORMAT_INVALID
	{
		let (mut backbuf_width, mut backbuf_height) = (0, 0);
		unsafe { SDL_GetWindowSizeInPixels(ctx.window, &mut backbuf_width, &mut backbuf_height) };
		ctx.setup_depth_texture(backbuf_width as u32, backbuf_height as u32, App::CREATE_DEPTH_BUFFER, 1.0)?;
	}

	// Start application
	let mut app = App::default();
	app.init(&ctx)?;

	let mut fullscreen = false;

	'quit: loop
	{
		unsafe
		{
			let mut event: SDL_Event = std::mem::zeroed();
			while SDL_PollEvent(addr_of_mut!(event))
			{
				match SDL_EventType(event.r#type)
				{
					SDL_EVENT_QUIT => break 'quit,
					SDL_EVENT_WINDOW_ENTER_FULLSCREEN => fullscreen = true,
					SDL_EVENT_WINDOW_LEAVE_FULLSCREEN => fullscreen = false,
					SDL_EVENT_KEY_DOWN | SDL_EVENT_KEY_UP => match event.key.key
					{
						SDLK_ESCAPE if event.key.down => break 'quit,
						SDLK_F1 if event.key.down => _ = SDL_SetWindowFullscreen(ctx.window, !fullscreen),
						_ => app.key(&ctx, event.key.key, event.key.down, event.key.repeat)
					}
					SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED => app.resize(&ctx, event.window.data1, event.window.data2),
					_ => ()
				}
			}
		}

		let cmd = unsafe { SDL_AcquireGPUCommandBuffer(ctx.device) };
		if cmd.is_null()
		{
			return Err(NeHeError::sdl("SDL_AcquireGPUCommandBuffer"));
		}

		let mut swapchain_texture_out = MaybeUninit::<*mut SDL_GPUTexture>::uninit();
		let (mut swapchain_width, mut swapchain_height) = (0, 0);
		if !unsafe { SDL_WaitAndAcquireGPUSwapchainTexture(cmd, ctx.window,
			swapchain_texture_out.as_mut_ptr(), &mut swapchain_width, &mut swapchain_height) }
		{
			let err = NeHeError::sdl("SDL_WaitAndAcquireGPUSwapchainTexture");
			unsafe { SDL_CancelGPUCommandBuffer(cmd); }
			return Err(err);
		}

		let swapchain_texture = unsafe { swapchain_texture_out.assume_init() };
		if swapchain_texture.is_null()
		{
			unsafe { SDL_CancelGPUCommandBuffer(cmd); }
			continue;
		}

		if App::CREATE_DEPTH_BUFFER != SDL_GPU_TEXTUREFORMAT_INVALID
			&& !ctx.depth_texture.is_null()
			&& ctx.depth_texture_size != (swapchain_width, swapchain_height)
		{
			ctx.setup_depth_texture(swapchain_width, swapchain_height, App::CREATE_DEPTH_BUFFER, 1.0)?;
		}

		app.draw(&ctx, cmd, swapchain_texture);
		unsafe { SDL_SubmitGPUCommandBuffer(cmd); }
	}

	// Cleanup & quit
	app.quit(&ctx);
	drop(ctx);
	unsafe { SDL_Quit() };
	Ok(())
}
