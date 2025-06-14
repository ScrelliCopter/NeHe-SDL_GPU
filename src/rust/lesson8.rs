/*
 * SPDX-FileCopyrightText: (C) 2025 a dinosaur
 * SPDX-License-Identifier: Zlib
 */

use nehe::application::config::AppImplementation;
use nehe::application::run;
use nehe::context::NeHeContext;
use nehe::error::NeHeError;
use nehe::matrix::Mtx;
use sdl3_sys::gpu::*;
use sdl3_sys::keyboard::SDL_GetKeyboardState;
use sdl3_sys::keycode::{SDL_Keycode, SDLK_B, SDLK_F, SDLK_L};
use sdl3_sys::pixels::SDL_FColor;
use sdl3_sys::scancode::{SDL_SCANCODE_DOWN, SDL_SCANCODE_LEFT, SDL_SCANCODE_PAGEDOWN, SDL_SCANCODE_PAGEUP, SDL_SCANCODE_RIGHT, SDL_SCANCODE_UP};
use std::cmp::max;
use std::ffi::c_void;
use std::mem::offset_of;
use std::process::ExitCode;
use std::ptr::{addr_of, null_mut};

#[repr(C)]
struct Vertex
{
	x: f32, y: f32, z: f32,
	nx: f32, ny: f32, nz: f32,
	u: f32, v: f32,
}

const VERTICES: &'static [Vertex] =
&[
	// Front Face
	Vertex { x: -1.0, y: -1.0, z:  1.0, nx:  0.0, ny:  0.0, nz:  1.0, u: 0.0, v: 0.0 },
	Vertex { x:  1.0, y: -1.0, z:  1.0, nx:  0.0, ny:  0.0, nz:  1.0, u: 1.0, v: 0.0 },
	Vertex { x:  1.0, y:  1.0, z:  1.0, nx:  0.0, ny:  0.0, nz:  1.0, u: 1.0, v: 1.0 },
	Vertex { x: -1.0, y:  1.0, z:  1.0, nx:  0.0, ny:  0.0, nz:  1.0, u: 0.0, v: 1.0 },
	// Back Face
	Vertex { x: -1.0, y: -1.0, z: -1.0, nx:  0.0, ny:  0.0, nz: -1.0, u: 1.0, v: 0.0 },
	Vertex { x: -1.0, y:  1.0, z: -1.0, nx:  0.0, ny:  0.0, nz: -1.0, u: 1.0, v: 1.0 },
	Vertex { x:  1.0, y:  1.0, z: -1.0, nx:  0.0, ny:  0.0, nz: -1.0, u: 0.0, v: 1.0 },
	Vertex { x:  1.0, y: -1.0, z: -1.0, nx:  0.0, ny:  0.0, nz: -1.0, u: 0.0, v: 0.0 },
	// Top Face
	Vertex { x: -1.0, y:  1.0, z: -1.0, nx:  0.0, ny:  1.0, nz:  0.0, u: 0.0, v: 1.0 },
	Vertex { x: -1.0, y:  1.0, z:  1.0, nx:  0.0, ny:  1.0, nz:  0.0, u: 0.0, v: 0.0 },
	Vertex { x:  1.0, y:  1.0, z:  1.0, nx:  0.0, ny:  1.0, nz:  0.0, u: 1.0, v: 0.0 },
	Vertex { x:  1.0, y:  1.0, z: -1.0, nx:  0.0, ny:  1.0, nz:  0.0, u: 1.0, v: 1.0 },
	// Bottom Face
	Vertex { x: -1.0, y: -1.0, z: -1.0, nx:  0.0, ny: -1.0, nz:  0.0, u: 1.0, v: 1.0 },
	Vertex { x:  1.0, y: -1.0, z: -1.0, nx:  0.0, ny: -1.0, nz:  0.0, u: 0.0, v: 1.0 },
	Vertex { x:  1.0, y: -1.0, z:  1.0, nx:  0.0, ny: -1.0, nz:  0.0, u: 0.0, v: 0.0 },
	Vertex { x: -1.0, y: -1.0, z:  1.0, nx:  0.0, ny: -1.0, nz:  0.0, u: 1.0, v: 0.0 },
	// Right face
	Vertex { x:  1.0, y: -1.0, z: -1.0, nx:  1.0, ny:  0.0, nz:  0.0, u: 1.0, v: 0.0 },
	Vertex { x:  1.0, y:  1.0, z: -1.0, nx:  1.0, ny:  0.0, nz:  0.0, u: 1.0, v: 1.0 },
	Vertex { x:  1.0, y:  1.0, z:  1.0, nx:  1.0, ny:  0.0, nz:  0.0, u: 0.0, v: 1.0 },
	Vertex { x:  1.0, y: -1.0, z:  1.0, nx:  1.0, ny:  0.0, nz:  0.0, u: 0.0, v: 0.0 },
	// Left Face
	Vertex { x: -1.0, y: -1.0, z: -1.0, nx: -1.0, ny:  0.0, nz:  0.0, u: 0.0, v: 0.0 },
	Vertex { x: -1.0, y: -1.0, z:  1.0, nx: -1.0, ny:  0.0, nz:  0.0, u: 1.0, v: 0.0 },
	Vertex { x: -1.0, y:  1.0, z:  1.0, nx: -1.0, ny:  0.0, nz:  0.0, u: 1.0, v: 1.0 },
	Vertex { x: -1.0, y:  1.0, z: -1.0, nx: -1.0, ny:  0.0, nz:  0.0, u: 0.0, v: 1.0 },
];

const INDICES: &'static [u16] =
&[
	 0,  1,  2,   2,  3,  0,  // Front
	 4,  5,  6,   6,  7,  4,  // Back
	 8,  9, 10,  10, 11,  8,  // Top
	12, 13, 14,  14, 15, 12,  // Bottom
	16, 17, 18,  18, 19, 16,  // Right
	20, 21, 22,  22, 23, 20   // Left
];

#[allow(dead_code)]
struct Light
{
	ambient: (f32, f32, f32, f32),
	diffuse: (f32, f32, f32, f32),
	position: (f32, f32, f32, f32),
}

struct Lesson8
{
	pso_unlit: *mut SDL_GPUGraphicsPipeline,
	pso_light: *mut SDL_GPUGraphicsPipeline,
	pso_blend_unlit: *mut SDL_GPUGraphicsPipeline,
	pso_blend_light: *mut SDL_GPUGraphicsPipeline,
	vtx_buffer: *mut SDL_GPUBuffer,
	idx_buffer: *mut SDL_GPUBuffer,
	samplers: [*mut SDL_GPUSampler; 3],
	texture: *mut SDL_GPUTexture,
	projection: Mtx,

	lighting: bool,
	blending: bool,
	light: Light,
	filter: usize,

	rot: (f32, f32),
	speed: (f32, f32),
	z: f32,
}

impl Default for Lesson8
{
	fn default() -> Self
	{
		Self
		{
			pso_unlit: null_mut(),
			pso_light: null_mut(),
			pso_blend_unlit: null_mut(),
			pso_blend_light: null_mut(),
			vtx_buffer: null_mut(),
			idx_buffer: null_mut(),
			samplers: [null_mut(); 3],
			texture: null_mut(),
			projection: Mtx::IDENTITY,

			lighting: false,
			blending: false,
			light: Light
			{
				ambient:  (0.5, 0.5, 0.5, 1.0),
				diffuse:  (1.0, 1.0, 1.0, 1.0),
				position: (0.0, 0.0, 2.0, 1.0),
			},
			filter: 0,

			rot: (0.0, 0.0),
			speed: (0.0, 0.0),
			z: -5.0,
		}
	}
}

impl AppImplementation for Lesson8
{
	const TITLE: &'static str = "Tom Stanis & NeHe's Blending Tutorial";
	const WIDTH: i32 = 640;
	const HEIGHT: i32 = 480;
	const CREATE_DEPTH_BUFFER: SDL_GPUTextureFormat = SDL_GPU_TEXTUREFORMAT_D16_UNORM;

	fn init(&mut self, ctx: &NeHeContext) -> Result<(), NeHeError>
	{
		let (vertex_shader_unlit, fragment_shader_unlit) = ctx.load_shaders("lesson8", 1, 0, 1)?;
		let (vertex_shader_light, fragment_shader_light) = ctx.load_shaders("lesson7", 2, 0, 1)?;

		const VERTEX_DESCRIPTIONS: &'static [SDL_GPUVertexBufferDescription] =
		&[
			SDL_GPUVertexBufferDescription
			{
				slot: 0,
				pitch: size_of::<Vertex>() as u32,
				input_rate: SDL_GPU_VERTEXINPUTRATE_VERTEX,
				instance_step_rate: 0,
			},
		];
		const VERTEX_ATTRIBS: &'static [SDL_GPUVertexAttribute] =
		&[
			SDL_GPUVertexAttribute
			{
				location: 0,
				buffer_slot: 0,
				format: SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3,
				offset: offset_of!(Vertex, x) as u32,
			},
			SDL_GPUVertexAttribute
			{
				location: 2,
				buffer_slot: 0,
				format: SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3,
				offset: offset_of!(Vertex, nx) as u32,
			},
			SDL_GPUVertexAttribute
			{
				location: 1,
				buffer_slot: 0,
				format: SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2,
				offset: offset_of!(Vertex, u) as u32,
			},
		];

		let mut pso_info = SDL_GPUGraphicsPipelineCreateInfo::default();
		pso_info.primitive_type  = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;
		pso_info.vertex_input_state = SDL_GPUVertexInputState
		{
			vertex_buffer_descriptions: VERTEX_DESCRIPTIONS.as_ptr(),
			num_vertex_buffers: VERTEX_DESCRIPTIONS.len() as u32,
			vertex_attributes: VERTEX_ATTRIBS.as_ptr(),
			num_vertex_attributes: VERTEX_ATTRIBS.len() as u32,
		};
		pso_info.rasterizer_state.fill_mode  = SDL_GPU_FILLMODE_FILL;
		pso_info.rasterizer_state.cull_mode  = SDL_GPU_CULLMODE_NONE;
		pso_info.rasterizer_state.front_face = SDL_GPU_FRONTFACE_COUNTER_CLOCKWISE;

		// Common pipeline depth & colour target options
		let mut color_targets = [ SDL_GPUColorTargetDescription::default() ];
		color_targets[0].format = unsafe { SDL_GetGPUSwapchainTextureFormat(ctx.device, ctx.window) };
		pso_info.target_info.color_target_descriptions = color_targets.as_ptr();
		pso_info.target_info.num_color_targets         = 1;
		pso_info.target_info.depth_stencil_format      = Self::CREATE_DEPTH_BUFFER;
		pso_info.target_info.has_depth_stencil_target  = true;
		pso_info.depth_stencil_state.compare_op        = SDL_GPU_COMPAREOP_LESS_OR_EQUAL;

		// Setup depth/stencil & colour pipeline state for no blending
		pso_info.depth_stencil_state.enable_depth_test  = true;
		pso_info.depth_stencil_state.enable_depth_write = true;
		color_targets[0].blend_state = unsafe { std::mem::zeroed() };

		// Create unlit pipeline
		pso_info.vertex_shader   = vertex_shader_unlit;
		pso_info.fragment_shader = fragment_shader_unlit;
		self.pso_unlit = unsafe { SDL_CreateGPUGraphicsPipeline(ctx.device, &pso_info).as_mut() }
			.ok_or(NeHeError::sdl("SDL_CreateGPUGraphicsPipeline"))?;

		// Create lit pipeline
		pso_info.vertex_shader   = vertex_shader_light;
		pso_info.fragment_shader = fragment_shader_light;
		self.pso_light = unsafe { SDL_CreateGPUGraphicsPipeline(ctx.device, &pso_info).as_mut() }
			.ok_or(NeHeError::sdl("SDL_CreateGPUGraphicsPipeline"))?;

		// Setup depth/stencil & colour pipeline state for blending
		pso_info.depth_stencil_state.enable_depth_test  = false;
		pso_info.depth_stencil_state.enable_depth_write = false;
		color_targets[0].blend_state.enable_blend = true;
		color_targets[0].blend_state.color_blend_op = SDL_GPU_BLENDOP_ADD;
		color_targets[0].blend_state.alpha_blend_op = SDL_GPU_BLENDOP_ADD;
		color_targets[0].blend_state.src_color_blendfactor = SDL_GPU_BLENDFACTOR_SRC_ALPHA;
		color_targets[0].blend_state.dst_color_blendfactor = SDL_GPU_BLENDFACTOR_ONE;
		color_targets[0].blend_state.src_alpha_blendfactor = SDL_GPU_BLENDFACTOR_SRC_ALPHA;
		color_targets[0].blend_state.dst_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE;

		// Create unlit blended pipeline
		pso_info.vertex_shader   = vertex_shader_unlit;
		pso_info.fragment_shader = fragment_shader_unlit;
		self.pso_blend_unlit = unsafe { SDL_CreateGPUGraphicsPipeline(ctx.device, &pso_info).as_mut() }
			.ok_or(NeHeError::sdl("SDL_CreateGPUGraphicsPipeline"))?;

		// Create lit blended pipeline
		pso_info.vertex_shader   = vertex_shader_light;
		pso_info.fragment_shader = fragment_shader_light;
		self.pso_blend_light = unsafe { SDL_CreateGPUGraphicsPipeline(ctx.device, &pso_info).as_mut() }
			.ok_or(NeHeError::sdl("SDL_CreateGPUGraphicsPipeline"))?;

		// Free shaders
		unsafe
		{
			SDL_ReleaseGPUShader(ctx.device, fragment_shader_light);
			SDL_ReleaseGPUShader(ctx.device, vertex_shader_light);
			SDL_ReleaseGPUShader(ctx.device, fragment_shader_unlit);
			SDL_ReleaseGPUShader(ctx.device, vertex_shader_unlit);
		}

		// Create texture samplers
		let create_sampler = |filter: SDL_GPUFilter, enable_mip: bool|
			-> Result<&mut SDL_GPUSampler, NeHeError>
		{
			let mut sampler_info = SDL_GPUSamplerCreateInfo::default();
			sampler_info.min_filter = filter;
			sampler_info.mag_filter = filter;
			sampler_info.mipmap_mode = SDL_GPU_SAMPLERMIPMAPMODE_NEAREST;
			sampler_info.max_lod = if enable_mip { f32::MAX } else { 0.0 };
			unsafe { SDL_CreateGPUSampler(ctx.device, &sampler_info).as_mut() }
				.ok_or(NeHeError::sdl("SDL_CreateGPUSampler"))
		};
		self.samplers[0] = create_sampler(SDL_GPU_FILTER_NEAREST, false)?;
		self.samplers[1] = create_sampler(SDL_GPU_FILTER_LINEAR, false)?;
		self.samplers[2] = create_sampler(SDL_GPU_FILTER_LINEAR, true)?;

		ctx.copy_pass(|pass|
		{
			self.texture = pass.load_texture("Data/Glass.bmp", true, true)?;
			self.vtx_buffer = pass.create_buffer(SDL_GPU_BUFFERUSAGE_VERTEX, VERTICES)?;
			self.idx_buffer = pass.create_buffer(SDL_GPU_BUFFERUSAGE_INDEX, INDICES)?;
			Ok(())
		})
	}

	fn quit(&mut self, ctx: &NeHeContext)
	{
		unsafe
		{
			SDL_ReleaseGPUBuffer(ctx.device, self.idx_buffer);
			SDL_ReleaseGPUBuffer(ctx.device, self.vtx_buffer);
			SDL_ReleaseGPUTexture(ctx.device, self.texture);
			self.samplers.iter().rev().for_each(|sampler| SDL_ReleaseGPUSampler(ctx.device, *sampler));
			SDL_ReleaseGPUGraphicsPipeline(ctx.device, self.pso_blend_light);
			SDL_ReleaseGPUGraphicsPipeline(ctx.device, self.pso_blend_unlit);
			SDL_ReleaseGPUGraphicsPipeline(ctx.device, self.pso_light);
			SDL_ReleaseGPUGraphicsPipeline(ctx.device, self.pso_unlit);
		}
	}

	fn resize(&mut self, _ctx: &NeHeContext, width: i32, height: i32)
	{
		let aspect = width as f32 / max(height, 1) as f32;
		self.projection = Mtx::perspective(45.0, aspect, 0.1, 100.0);
	}

	fn draw(&mut self, ctx: &NeHeContext, cmd: *mut SDL_GPUCommandBuffer, swapchain: *mut SDL_GPUTexture)
	{
		let mut color_info = SDL_GPUColorTargetInfo::default();
		color_info.texture     = swapchain;
		color_info.clear_color = SDL_FColor { r: 0.0, g: 0.0, b: 0.0, a: 0.5 };
		color_info.load_op     = SDL_GPU_LOADOP_CLEAR;
		color_info.store_op    = SDL_GPU_STOREOP_STORE;

		let mut depth_info = SDL_GPUDepthStencilTargetInfo::default();
		depth_info.texture          = ctx.depth_texture;
		depth_info.clear_depth      = 1.0;
		depth_info.load_op          = SDL_GPU_LOADOP_CLEAR;
		depth_info.store_op         = SDL_GPU_STOREOP_DONT_CARE;
		depth_info.stencil_load_op  = SDL_GPU_LOADOP_DONT_CARE;
		depth_info.stencil_store_op = SDL_GPU_STOREOP_DONT_CARE;
		depth_info.cycle            = true;

		unsafe
		{
			// Begin pass & bind pipeline state
			let pass = SDL_BeginGPURenderPass(cmd, &color_info, 1, &depth_info);
			SDL_BindGPUGraphicsPipeline(pass, match (self.blending, self.lighting)
			{
				(false, false) => self.pso_unlit,
				(false, true)  => self.pso_light,
				(true,  false) => self.pso_blend_unlit,
				(true,  true)  => self.pso_blend_light,
			});

			// Bind texture
			let texture_binding = SDL_GPUTextureSamplerBinding
			{
				texture: self.texture,
				sampler: self.samplers[self.filter],
			};
			SDL_BindGPUFragmentSamplers(pass, 0, &texture_binding, 1);

			// Bind vertex & index buffers
			let vtx_binding = SDL_GPUBufferBinding { buffer: self.vtx_buffer, offset: 0 };
			let idx_binding = SDL_GPUBufferBinding { buffer: self.idx_buffer, offset: 0 };
			SDL_BindGPUVertexBuffers(pass, 0, &vtx_binding, 1);
			SDL_BindGPUIndexBuffer(pass, &idx_binding, SDL_GPU_INDEXELEMENTSIZE_16BIT);

			// Setup the cube's model matrix
			let mut model = Mtx::translation(0.0, 0.0, self.z);
			model.rotate(self.rot.0, 1.0, 0.0, 0.0);
			model.rotate(self.rot.1, 0.0, 1.0, 0.0);

			// Push shader uniforms
			if self.lighting
			{
				#[allow(dead_code)]
				struct Uniforms { model: Mtx, projection: Mtx }
				let u = Uniforms { model, projection: self.projection };
				SDL_PushGPUVertexUniformData(cmd, 0, addr_of!(u) as *const c_void, size_of::<Uniforms>() as u32);
				SDL_PushGPUVertexUniformData(cmd, 1, addr_of!(self.light) as *const c_void, size_of::<Light>() as u32);
			}
			else
			{
				#[allow(dead_code)]
				struct Uniforms { model_view_proj: Mtx, color: [f32; 4] }
				let color = [1.0, 1.0, 1.0, 0.5];  // 50% translucency
				let u = Uniforms { model_view_proj: self.projection * model, color };
				SDL_PushGPUVertexUniformData(cmd, 0, addr_of!(u) as *const c_void, size_of::<Uniforms>() as u32);
			}

			// Draw the textured cube
			SDL_DrawGPUIndexedPrimitives(pass, INDICES.len() as u32, 1, 0, 0, 0);

			SDL_EndGPURenderPass(pass);
		}

		let keys = unsafe
		{
			let mut numkeys: std::ffi::c_int = 0;
			let keys = SDL_GetKeyboardState(&mut numkeys);
			std::slice::from_raw_parts(keys, numkeys as usize)
		};
		if keys[SDL_SCANCODE_PAGEUP.0 as usize] { self.z -= 0.02 }
		if keys[SDL_SCANCODE_PAGEDOWN.0 as usize] { self.z += 0.02; }
		if keys[SDL_SCANCODE_UP.0 as usize]   { self.speed.0 -= 0.01; }
		if keys[SDL_SCANCODE_DOWN.0 as usize] { self.speed.0 += 0.01; }
		if keys[SDL_SCANCODE_RIGHT.0 as usize] { self.speed.1 += 0.1; }
		if keys[SDL_SCANCODE_LEFT.0 as usize]  { self.speed.1 -= 0.1; }

		self.rot.0 += self.speed.0;
		self.rot.1 += self.speed.1;
	}

	fn key(&mut self, _ctx: &NeHeContext, key: SDL_Keycode, down: bool, _repeat: bool)
	{
		match key
		{
			SDLK_L if down => self.lighting = !self.lighting,
			SDLK_B if down => self.blending = !self.blending,
			SDLK_F if down => self.filter = (self.filter + 1) % self.samplers.len(),
			_ => ()
		}
	}
}

pub fn main() -> Result<ExitCode, Box<dyn std::error::Error>>
{
	run::<Lesson8>()?;
	Ok(ExitCode::SUCCESS)
}
