/*
 * SPDX-FileCopyrightText: (C) 2025 a dinosaur
 * SPDX-License-Identifier: Zlib
 */

mod copypass;

use crate::context::copypass::NeHeCopyPass;
use crate::error::NeHeError;
use sdl3_sys::filesystem::SDL_GetBasePath;
use sdl3_sys::gpu::*;
use sdl3_sys::properties::{SDL_CreateProperties, SDL_DestroyProperties, SDL_SetFloatProperty};
use sdl3_sys::video::*;
use std::ffi::{CStr, CString};
use std::fs;
use std::path::{Path, PathBuf};
use std::ptr::{null, null_mut};
use std::str::from_utf8;

pub struct NeHeContext
{
	pub window: *mut SDL_Window,
	pub device: *mut SDL_GPUDevice,
	pub depth_texture: *mut SDL_GPUTexture,
	pub depth_texture_size: (u32, u32)
}

impl NeHeContext
{
	#[allow(unsafe_op_in_unsafe_fn)]
	pub(in crate) fn init(title: &str, w: i32, h: i32) -> Result<Self, NeHeError>
	{
		// Create window
		let flags = SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY;
		let window = unsafe
		{
			let title = CString::new(title).unwrap();
			SDL_CreateWindow(title.as_ptr(), w, h, flags)
		};
		if window.is_null()
		{
			return Err(NeHeError::sdl("SDL_CreateWindow"));
		}

		// Open GPU device
		let formats = SDL_GPU_SHADERFORMAT_METALLIB | SDL_GPU_SHADERFORMAT_MSL |
			SDL_GPU_SHADERFORMAT_SPIRV | SDL_GPU_SHADERFORMAT_DXIL;
		let Some(device) = (unsafe { SDL_CreateGPUDevice(formats, true, null()).as_mut() }) else
		{
			return Err(NeHeError::sdl("SDL_CreateGPUDevice"));
		};

		// Attach window to the GPU device
		if !unsafe { SDL_ClaimWindowForGPUDevice(device, window) }
		{
			return Err(NeHeError::sdl("SDL_ClaimWindowForGPUDevice"));
		}

		// Enable VSync
		unsafe
		{
			SDL_SetGPUSwapchainParameters(device, window,
				SDL_GPU_SWAPCHAINCOMPOSITION_SDR,
				SDL_GPU_PRESENTMODE_VSYNC);
		}

		Ok(Self
		{
			window, device,
			depth_texture: null_mut(),
			depth_texture_size: (0, 0),
		})
	}
}

impl Drop for NeHeContext
{
	fn drop(&mut self)
	{
		unsafe
		{
			if !self.depth_texture.is_null()
			{
				SDL_ReleaseGPUTexture(self.device, self.depth_texture);
			}
			SDL_ReleaseWindowFromGPUDevice(self.device, self.window);
			SDL_DestroyGPUDevice(self.device);
			SDL_DestroyWindow(self.window);
		}
	}
}

#[allow(dead_code)]
impl NeHeContext
{
	pub fn setup_depth_texture(&mut self, width: u32, height: u32, format: SDL_GPUTextureFormat, clear_depth: f32) -> Result<(), NeHeError>
	{
		if !self.depth_texture.is_null()
		{
			unsafe { SDL_ReleaseGPUTexture(self.device, self.depth_texture) };
			self.depth_texture = null_mut();
		}

		let props = unsafe { SDL_CreateProperties() };
		if props == 0
		{
			return Err(NeHeError::sdl("SDL_CreateProperties"));
		}
		// Workaround for https://github.com/libsdl-org/SDL/issues/10758
		unsafe { SDL_SetFloatProperty(props, SDL_PROP_GPU_TEXTURE_CREATE_D3D12_CLEAR_DEPTH_FLOAT, clear_depth) };

		let info = SDL_GPUTextureCreateInfo
		{
			r#type: SDL_GPU_TEXTURETYPE_2D,
			format,
			usage: SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET,
			width,
			height,
			layer_count_or_depth: 1,
			num_levels: 1,
			sample_count: SDL_GPU_SAMPLECOUNT_1,
			props,
		};
		let texture = unsafe { SDL_CreateGPUTexture(self.device, &info) };
		unsafe { SDL_DestroyProperties(props) };
		if texture.is_null()
		{
			return Err(NeHeError::sdl("SDL_CreateGPUTexture"));
		}

		self.depth_texture = texture;
		self.depth_texture_size = (width, height);
		Ok(())
	}

	pub fn load_shaders(&self, name: &str,
		vertex_uniforms: u32,
		vertex_storage: u32,
		fragment_samplers: u32)
		-> Result<(*mut SDL_GPUShader, *mut SDL_GPUShader), NeHeError>
	{
		let base = unsafe { CStr::from_ptr(SDL_GetBasePath()) };
		let path = Path::new(from_utf8(base.to_bytes()).unwrap())
			.join("Data/Shaders").join(name);

		let mut info = ShaderProgramCreateInfo
		{
			format: SDL_GPU_SHADERFORMAT_INVALID,
			vertex_uniforms, vertex_storage, fragment_samplers
		};

		let available_formats = unsafe { SDL_GetGPUShaderFormats(self.device) };
		if available_formats & (SDL_GPU_SHADERFORMAT_METALLIB | SDL_GPU_SHADERFORMAT_MSL) != 0
		{
			if available_formats & SDL_GPU_SHADERFORMAT_METALLIB == SDL_GPU_SHADERFORMAT_METALLIB
			{
				if let Ok(lib) = fs::read(path.appending_ext("metallib"))
				{
					info.format = SDL_GPU_SHADERFORMAT_METALLIB;
					return Ok((
						self.load_shader_blob(&lib, &info, SDL_GPU_SHADERSTAGE_VERTEX, "VertexMain")?,
						self.load_shader_blob(&lib, &info, SDL_GPU_SHADERSTAGE_FRAGMENT, "FragmentMain")?
					));
				}
			}
			if available_formats & SDL_GPU_SHADERFORMAT_MSL == SDL_GPU_SHADERFORMAT_MSL
			{
				let src = fs::read(path.appending_ext("metal"))?;
				info.format = SDL_GPU_SHADERFORMAT_MSL;
				return Ok((
					self.load_shader_blob(&src, &info, SDL_GPU_SHADERSTAGE_VERTEX, "VertexMain")?,
					self.load_shader_blob(&src, &info, SDL_GPU_SHADERSTAGE_FRAGMENT, "FragmentMain")?
				));
			}
			unreachable!();
		}
		else if available_formats & SDL_GPU_SHADERFORMAT_SPIRV == SDL_GPU_SHADERFORMAT_SPIRV
		{
			info.format = SDL_GPU_SHADERFORMAT_SPIRV;
			Ok((
				self.load_shader(path.appending_ext("vtx.spv"), &info, SDL_GPU_SHADERSTAGE_VERTEX, "VertexMain")?,
				self.load_shader(path.appending_ext("frg.spv"), &info, SDL_GPU_SHADERSTAGE_FRAGMENT, "FragmentMain")?
			))
		}
		else if available_formats & SDL_GPU_SHADERFORMAT_DXIL == SDL_GPU_SHADERFORMAT_DXIL
		{
			info.format = SDL_GPU_SHADERFORMAT_DXIL;
			Ok((
				self.load_shader(path.appending_ext("vtx.dxb"), &info, SDL_GPU_SHADERSTAGE_VERTEX, "VertexMain")?,
				self.load_shader(path.appending_ext("pxl.dxb"), &info, SDL_GPU_SHADERSTAGE_FRAGMENT, "PixelMain")?
			))
		}
		else { Err(NeHeError::Fatal("No supported shader formats")) }
	}

	fn load_shader(&self, filepath: PathBuf,
		info: &ShaderProgramCreateInfo, stage: SDL_GPUShaderStage,
		main: &str) -> Result<&mut SDL_GPUShader, NeHeError>
	{
		let data = fs::read(filepath)?;
		self.load_shader_blob(&data, info, stage, main)
	}

	fn load_shader_blob(&self, code: &Vec<u8>,
		info: &ShaderProgramCreateInfo, stage: SDL_GPUShaderStage,
		main: &str) -> Result<&mut SDL_GPUShader, NeHeError>
	{
		let Ok(entrypoint) = CString::new(main) else
		{
			return Err(NeHeError::Fatal("Null dereference when converting entrypoint string"));
		};
		let info = SDL_GPUShaderCreateInfo
		{
			code_size: code.len(),
			code: code.as_ptr(),
			entrypoint: entrypoint.as_ptr(),
			format: info.format,
			stage,
			num_samplers: if stage == SDL_GPU_SHADERSTAGE_FRAGMENT { info.fragment_samplers } else { 0 },
			num_storage_textures: 0,
			num_storage_buffers: if stage == SDL_GPU_SHADERSTAGE_VERTEX { info.vertex_storage } else { 0 },
			num_uniform_buffers: if stage == SDL_GPU_SHADERSTAGE_VERTEX { info.vertex_uniforms } else { 0 },
			props: 0,
		};
		unsafe { SDL_CreateGPUShader(self.device, &info).as_mut() }
			.ok_or(NeHeError::sdl("SDL_CreateGPUShader"))
	}

	pub fn copy_pass(&self, lambda: impl FnOnce(&mut NeHeCopyPass) -> Result<(), NeHeError>) -> Result<(), NeHeError>
	{
		let mut pass = NeHeCopyPass::new(&self);
		lambda(&mut pass)?;
		pass.submit()
	}
}

struct ShaderProgramCreateInfo
{
	format: SDL_GPUShaderFormat,
	vertex_uniforms: u32,
	vertex_storage: u32,
	fragment_samplers: u32,
}

trait PathExt
{
	fn appending_ext(&self, ext: &str) -> Self;
}

impl PathExt for PathBuf
{
	fn appending_ext(&self, ext: &str) -> PathBuf
	{
		let mut path = self.as_os_str().to_owned();
		path.push(".");
		path.push(ext);
		path.into()
	}
}
