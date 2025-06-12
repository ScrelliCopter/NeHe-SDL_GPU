/*
 * SPDX-FileCopyrightText: (C) 2025 a dinosaur
 * SPDX-License-Identifier: Zlib
 */

use nehe::application::config::AppImplementation;
use nehe::application::run;
use nehe::context::NeHeContext;
use sdl3_sys::gpu::*;
use sdl3_sys::pixels::SDL_FColor;
use std::process::ExitCode;
use std::ptr::null_mut;

#[derive(Default)]
struct Lesson1;

impl AppImplementation for Lesson1
{
	const TITLE: &'static str = "NeHe's OpenGL Framework";
	const WIDTH: i32 = 640;
	const HEIGHT: i32 = 480;

	fn draw(&mut self, _ctx: &NeHeContext, cmd: *mut SDL_GPUCommandBuffer, swapchain: *mut SDL_GPUTexture)
	{
		let mut color_info = SDL_GPUColorTargetInfo::default();
		color_info.texture     = swapchain;
		color_info.clear_color = SDL_FColor { r: 0.0, g: 0.0, b: 0.0, a: 0.5 };
		color_info.load_op     = SDL_GPU_LOADOP_CLEAR;
		color_info.store_op    = SDL_GPU_STOREOP_STORE;

		unsafe
		{
			let pass = SDL_BeginGPURenderPass(cmd, &color_info, 1, null_mut());
			SDL_EndGPURenderPass(pass);
		}
	}
}

pub fn main() -> Result<ExitCode, Box<dyn std::error::Error>>
{
	run::<Lesson1>()?;
	Ok(ExitCode::SUCCESS)
}
