/*
 * SPDX-FileCopyrightText: (C) 2025 a dinosaur
 * SPDX-License-Identifier: Zlib
 */

use crate::context::NeHeContext;
use crate::error::NeHeError;
use sdl3_sys::filesystem::SDL_GetBasePath;
use sdl3_sys::gpu::*;
use sdl3_sys::pixels::*;
use sdl3_sys::surface::{SDL_ConvertSurface, SDL_DestroySurface, SDL_FlipSurface, SDL_LoadBMP, SDL_Surface, SDL_FLIP_VERTICAL};
use std::cmp::max;
use std::ffi::{CStr, CString};
use std::ptr;

pub struct NeHeCopyPass<'a>
{
	ctx: &'a NeHeContext,
	copies: Vec<Copy>,
}

impl NeHeCopyPass<'_>
{
	pub fn create_buffer<E>(&mut self, usage: SDL_GPUBufferUsageFlags, elements: &[E])
		-> Result<*mut SDL_GPUBuffer, NeHeError>
	{
		let size = (size_of::<E>() * elements.len()) as u32;

		// Create data buffer
		let info = SDL_GPUBufferCreateInfo { usage, size, props: 0 };
		let buffer = unsafe { SDL_CreateGPUBuffer(self.ctx.device, &info).as_mut() }
			.ok_or(NeHeError::sdl("SDL_CreateGPUBuffer"))?;

		// Create transfer buffer
		let xfer_info = SDL_GPUTransferBufferCreateInfo { usage: SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD, size, props: 0 };
		let Some(transfer_buffer) = (unsafe { SDL_CreateGPUTransferBuffer(self.ctx.device, &xfer_info).as_mut() }) else
		{
			let err = NeHeError::sdl("SDL_CreateGPUTransferBuffer");
			unsafe { SDL_ReleaseGPUBuffer(self.ctx.device, buffer); }
			return Err(err);
		};

		// Map transfer buffer and copy the data
		unsafe
		{
			let map = SDL_MapGPUTransferBuffer(self.ctx.device, transfer_buffer, false);
			if map.is_null()
			{
				let err = NeHeError::sdl("SDL_MapGPUTransferBuffer");
				SDL_ReleaseGPUTransferBuffer(self.ctx.device, transfer_buffer);
				SDL_ReleaseGPUBuffer(self.ctx.device, buffer);
				return Err(err);
			}
			ptr::copy_nonoverlapping(elements.as_ptr(), map as *mut E, elements.len());
			SDL_UnmapGPUTransferBuffer(self.ctx.device, transfer_buffer);
		}

		self.copies.push(Copy { transfer_buffer, payload: CopyPayload::Buffer { buffer, size } });
		Ok(buffer)
	}

	pub fn load_texture(&mut self, resource_path: &str, flip_vertical: bool, gen_mipmaps: bool)
		-> Result<&mut SDL_GPUTexture, NeHeError>
	{
		// Build path to resource: "{baseDir}/{resourcePath}"
		let path = unsafe
		{
			let mut path = CString::from(CStr::from_ptr(SDL_GetBasePath())).into_bytes();
			path.extend_from_slice(resource_path.as_bytes());
			CString::from_vec_unchecked(path)
		};

		// Load image into a surface
		let image = unsafe { SDL_LoadBMP(path.as_ptr()).as_mut() }
			.ok_or(NeHeError::sdl("SDL_LoadBMP"))?;

		// Flip surface if requested
		if flip_vertical && !unsafe { SDL_FlipSurface(image, SDL_FLIP_VERTICAL) }
		{
			let err = NeHeError::sdl("SDL_FlipSurface");
			unsafe { SDL_DestroySurface(image); }
			return Err(err);
		}

		// Upload texture to the GPU
		let result = self.create_texture_from_surface(image, gen_mipmaps);
		unsafe { SDL_DestroySurface(image); }
		return result;
	}

	fn create_texture_from_pixels(&mut self, pixels: &[u8], create_info: SDL_GPUTextureCreateInfo, gen_mipmaps: bool)
		-> Result<&mut SDL_GPUTexture, NeHeError>
	{
		assert!(pixels.len() <= u32::MAX as usize);

		let texture = unsafe { SDL_CreateGPUTexture(self.ctx.device, &create_info).as_mut() }
			.ok_or(NeHeError::sdl("SDL_CreateGPUTexture"))?;

		// Create and copy image data to a transfer buffer
		let xfer_info = SDL_GPUTransferBufferCreateInfo
		{
			usage: SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
			size: pixels.len() as u32,
			props: 0
		};
		let Some(transfer_buffer) = (unsafe { SDL_CreateGPUTransferBuffer(self.ctx.device, &xfer_info).as_mut() }) else
		{
			let err = NeHeError::sdl("SDL_CreateGPUTransferBuffer");
			unsafe { SDL_ReleaseGPUTexture(self.ctx.device, texture); }
			return Err(err);
		};

		// Map transfer buffer and copy the data
		unsafe
		{
			let map = SDL_MapGPUTransferBuffer(self.ctx.device, transfer_buffer, false);
			if map.is_null()
			{
				let err = NeHeError::sdl("SDL_MapGPUTransferBuffer");
				SDL_ReleaseGPUTransferBuffer(self.ctx.device, transfer_buffer);
				SDL_ReleaseGPUTexture(self.ctx.device, texture);
				return Err(err);
			}
			ptr::copy_nonoverlapping(pixels.as_ptr(), map as *mut u8, pixels.len());
			SDL_UnmapGPUTransferBuffer(self.ctx.device, transfer_buffer);
		}

		self.copies.push(Copy { transfer_buffer, payload: CopyPayload::Texture
		{
			texture,
			size: (create_info.width, create_info.height),
			gen_mipmaps
		} });
		Ok(texture)
	}

	pub fn create_texture_from_surface(&mut self, surface: &mut SDL_Surface, gen_mipmaps: bool)
		-> Result<&mut SDL_GPUTexture, NeHeError>
	{
		let mut info: SDL_GPUTextureCreateInfo = unsafe { std::mem::zeroed() };
		info.r#type = SDL_GPU_TEXTURETYPE_2D;
		info.format = SDL_GPU_TEXTUREFORMAT_INVALID;
		info.usage  = SDL_GPU_TEXTUREUSAGE_SAMPLER;
		info.width  = surface.w as u32;
		info.height = surface.h as u32;
		info.layer_count_or_depth = 1;
		info.num_levels           = 1;

		let needs_convert: bool;
		(needs_convert, info.format) = match surface.format
		{
			SDL_PIXELFORMAT_RGBA32        => (false, SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM),
			SDL_PIXELFORMAT_RGBA64        => (false, SDL_GPU_TEXTUREFORMAT_R16G16B16A16_UNORM),
			SDL_PIXELFORMAT_RGB565        => (false, SDL_GPU_TEXTUREFORMAT_B5G6R5_UNORM),
			SDL_PIXELFORMAT_ARGB1555      => (false, SDL_GPU_TEXTUREFORMAT_B5G5R5A1_UNORM),
			SDL_PIXELFORMAT_BGRA4444      => (false, SDL_GPU_TEXTUREFORMAT_B4G4R4A4_UNORM),
			SDL_PIXELFORMAT_BGRA32        => (false, SDL_GPU_TEXTUREFORMAT_B8G8R8A8_UNORM),
			SDL_PIXELFORMAT_RGBA64_FLOAT  => (false, SDL_GPU_TEXTUREFORMAT_R16G16B16A16_FLOAT),
			SDL_PIXELFORMAT_RGBA128_FLOAT => (false, SDL_GPU_TEXTUREFORMAT_R32G32B32A32_FLOAT),
			_ =>                             (true, SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM)
		};

		if gen_mipmaps
		{
			info.usage |= SDL_GPU_TEXTUREUSAGE_COLOR_TARGET;
			info.num_levels = max(info.width, info.height).ilog2() + 1;  // floor(log₂(max(𝑤,ℎ))) + 1
		}

		if needs_convert
		{
			// Convert pixel format if required
			let conv = unsafe { SDL_ConvertSurface(surface, SDL_PIXELFORMAT_ABGR8888).as_mut() }
				.ok_or(NeHeError::sdl("SDL_ConvertSurface"))?;
			let result = self.create_texture_from_pixels(
				unsafe { std::slice::from_raw_parts(
					conv.pixels as *const u8,
					conv.pitch as usize * conv.h as usize) },
				info,
				gen_mipmaps);
			unsafe { SDL_DestroySurface(conv); }
			result
		}
		else
		{
			self.create_texture_from_pixels(
				unsafe { std::slice::from_raw_parts(
					surface.pixels as *const u8,
					surface.pitch as usize * surface.h as usize) },
				info,
				gen_mipmaps)
		}
	}
}

impl NeHeCopyPass<'_>
{
	pub(in crate::context) fn new(ctx: &'_ NeHeContext) -> NeHeCopyPass<'_>
	{
		NeHeCopyPass { ctx, copies: Vec::new() }
	}

	pub(in crate::context) fn submit(&mut self, ) -> Result<(), NeHeError>
	{
		let cmd = unsafe { SDL_AcquireGPUCommandBuffer(self.ctx.device).as_mut() }
			.ok_or(NeHeError::sdl("SDL_AcquireGPUCommandBuffer"))?;

		// Begin the copy pass
		let pass = unsafe { SDL_BeginGPUCopyPass(cmd) };

		// Upload data into GPU buffer(s)
		for copy in self.copies.iter()
		{
			match copy.payload
			{
				CopyPayload::Buffer { buffer, size } =>
				{
					let source = SDL_GPUTransferBufferLocation { transfer_buffer: copy.transfer_buffer, offset: 0 };
					let destination = SDL_GPUBufferRegion { buffer, offset: 0, size };
					unsafe { SDL_UploadToGPUBuffer(pass, &source, &destination, false); }
				}
				CopyPayload::Texture { texture, size, .. } =>
				{
					let mut source: SDL_GPUTextureTransferInfo = unsafe { std::mem::zeroed() };
					source.transfer_buffer = copy.transfer_buffer;
					source.offset = 0;
					let mut destination: SDL_GPUTextureRegion = unsafe { std::mem::zeroed() };
					destination.texture = texture;
					destination.w = size.0;
					destination.h = size.1;
					destination.d = 1;  // info.layer_count_or_depth
					unsafe { SDL_UploadToGPUTexture(pass, &source, &destination, false); }
				}
			}
		}

		// End the copy pass
		unsafe { SDL_EndGPUCopyPass(pass); }

		// Generate mipmaps if needed
		self.copies.iter().for_each(|copy| match copy.payload
		{
			CopyPayload::Texture { texture, gen_mipmaps, .. } if gen_mipmaps
				=> unsafe { SDL_GenerateMipmapsForGPUTexture(cmd, texture) },
			_ => ()
		});

		// Submit the command buffer
		unsafe { SDL_SubmitGPUCommandBuffer(cmd); }
		Ok(())
	}
}

impl Drop for NeHeCopyPass<'_>
{
	fn drop(&mut self)
	{
		for copy in self.copies.iter().rev()
		{
			unsafe { SDL_ReleaseGPUTransferBuffer(self.ctx.device, copy.transfer_buffer) };
		}
	}
}

struct Copy
{
	transfer_buffer: *mut SDL_GPUTransferBuffer,
	payload: CopyPayload,
}

enum CopyPayload
{
	Buffer { buffer: *mut SDL_GPUBuffer, size: u32 },
	Texture { texture: *mut SDL_GPUTexture, size: (u32, u32), gen_mipmaps: bool },
}
