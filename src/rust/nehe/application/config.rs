/*
 * SPDX-FileCopyrightText: (C) 2025 a dinosaur
 * SPDX-License-Identifier: Zlib
 */

use sdl3_sys::gpu::{SDL_GPUCommandBuffer, SDL_GPUTexture, SDL_GPUTextureFormat, SDL_GPU_TEXTUREFORMAT_INVALID};
use sdl3_sys::keycode::SDL_Keycode;
use crate::context::NeHeContext;
use crate::error::NeHeError;

#[allow(unused_variables)]
pub trait AppImplementation
{
	const TITLE: &'static str;
	const WIDTH: i32;
	const HEIGHT: i32;
	const CREATE_DEPTH_BUFFER: SDL_GPUTextureFormat = SDL_GPU_TEXTUREFORMAT_INVALID;

	fn init(&mut self, ctx: &NeHeContext) -> Result<(), NeHeError> { Ok(()) }
	fn quit(&mut self, ctx: &NeHeContext) {}
	fn resize(&mut self, ctx: &NeHeContext, width: i32, height: i32) {}
	fn draw(&mut self, ctx: &NeHeContext, cmd: *mut SDL_GPUCommandBuffer, swapchain: *mut SDL_GPUTexture);
	fn key(&mut self, ctx: &NeHeContext, key: SDL_Keycode, down: bool, repeat: bool) {}
}
