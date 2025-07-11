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
use sdl3_sys::pixels::SDL_FColor;
use std::cmp::max;
use std::ffi::c_void;
use std::mem::offset_of;
use std::process::ExitCode;
use std::ptr::{addr_of, null_mut};

#[repr(C)]
struct Vertex
{
	x: f32, y: f32, z: f32,
	u: f32, v: f32,
}

const VERTICES: &'static [Vertex] =
&[
	// Front Face
	Vertex { x: -1.0, y: -1.0, z:  1.0, u: 0.0, v: 0.0 },
	Vertex { x:  1.0, y: -1.0, z:  1.0, u: 1.0, v: 0.0 },
	Vertex { x:  1.0, y:  1.0, z:  1.0, u: 1.0, v: 1.0 },
	Vertex { x: -1.0, y:  1.0, z:  1.0, u: 0.0, v: 1.0 },
	// Back Face
	Vertex { x: -1.0, y: -1.0, z: -1.0, u: 1.0, v: 0.0 },
	Vertex { x: -1.0, y:  1.0, z: -1.0, u: 1.0, v: 1.0 },
	Vertex { x:  1.0, y:  1.0, z: -1.0, u: 0.0, v: 1.0 },
	Vertex { x:  1.0, y: -1.0, z: -1.0, u: 0.0, v: 0.0 },
	// Top Face
	Vertex { x: -1.0, y:  1.0, z: -1.0, u: 0.0, v: 1.0 },
	Vertex { x: -1.0, y:  1.0, z:  1.0, u: 0.0, v: 0.0 },
	Vertex { x:  1.0, y:  1.0, z:  1.0, u: 1.0, v: 0.0 },
	Vertex { x:  1.0, y:  1.0, z: -1.0, u: 1.0, v: 1.0 },
	// Bottom Face
	Vertex { x: -1.0, y: -1.0, z: -1.0, u: 1.0, v: 1.0 },
	Vertex { x:  1.0, y: -1.0, z: -1.0, u: 0.0, v: 1.0 },
	Vertex { x:  1.0, y: -1.0, z:  1.0, u: 0.0, v: 0.0 },
	Vertex { x: -1.0, y: -1.0, z:  1.0, u: 1.0, v: 0.0 },
	// Right face
	Vertex { x:  1.0, y: -1.0, z: -1.0, u: 1.0, v: 0.0 },
	Vertex { x:  1.0, y:  1.0, z: -1.0, u: 1.0, v: 1.0 },
	Vertex { x:  1.0, y:  1.0, z:  1.0, u: 0.0, v: 1.0 },
	Vertex { x:  1.0, y: -1.0, z:  1.0, u: 0.0, v: 0.0 },
	// Left Face
	Vertex { x: -1.0, y: -1.0, z: -1.0, u: 0.0, v: 0.0 },
	Vertex { x: -1.0, y: -1.0, z:  1.0, u: 1.0, v: 0.0 },
	Vertex { x: -1.0, y:  1.0, z:  1.0, u: 1.0, v: 1.0 },
	Vertex { x: -1.0, y:  1.0, z: -1.0, u: 0.0, v: 1.0 },
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

struct Lesson6
{
	pso: *mut SDL_GPUGraphicsPipeline,
	vtx_buffer: *mut SDL_GPUBuffer,
	idx_buffer: *mut SDL_GPUBuffer,
	sampler: *mut SDL_GPUSampler,
	texture: *mut SDL_GPUTexture,
	projection: Mtx,

	rot: (f32, f32, f32),
}

//FIXME: remove when `raw_ptr_default`
impl Default for Lesson6
{
	fn default() -> Self
	{
		Self
		{
			pso: null_mut(),
			vtx_buffer: null_mut(),
			idx_buffer: null_mut(),
			sampler: null_mut(),
			texture: null_mut(),
			projection: Mtx::IDENTITY,

			rot: (0.0, 0.0, 0.0),
		}
	}
}

impl AppImplementation for Lesson6
{
	const TITLE: &'static str = "NeHe's Texture Mapping Tutorial";
	const WIDTH: i32 = 640;
	const HEIGHT: i32 = 480;
	const CREATE_DEPTH_BUFFER: SDL_GPUTextureFormat = SDL_GPU_TEXTUREFORMAT_D16_UNORM;

	fn init(&mut self, ctx: &NeHeContext) -> Result<(), NeHeError>
	{
		let (vertex_shader, fragment_shader) = ctx.load_shaders("lesson6", 1, 0, 1)?;

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
				location: 1,
				buffer_slot: 0,
				format: SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2,
				offset: offset_of!(Vertex, u) as u32,
			},
		];

		let mut pso_info = SDL_GPUGraphicsPipelineCreateInfo::default();
		pso_info.vertex_shader   = vertex_shader;
		pso_info.fragment_shader = fragment_shader;
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
		let color_targets =
		[
			SDL_GPUColorTargetDescription
			{
				format: unsafe { SDL_GetGPUSwapchainTextureFormat(ctx.device, ctx.window) },
				blend_state: SDL_GPUColorTargetBlendState::default(),
			}
		];
		pso_info.target_info.color_target_descriptions = color_targets.as_ptr();
		pso_info.target_info.num_color_targets         = color_targets.len() as u32;
		pso_info.target_info.depth_stencil_format      = SDL_GPU_TEXTUREFORMAT_D16_UNORM;
		pso_info.target_info.has_depth_stencil_target  = true;

		pso_info.depth_stencil_state.compare_op         = SDL_GPU_COMPAREOP_LESS_OR_EQUAL;
		pso_info.depth_stencil_state.enable_depth_test  = true;
		pso_info.depth_stencil_state.enable_depth_write = true;

		self.pso = unsafe { SDL_CreateGPUGraphicsPipeline(ctx.device, &pso_info) };
		unsafe
		{
			SDL_ReleaseGPUShader(ctx.device, fragment_shader);
			SDL_ReleaseGPUShader(ctx.device, vertex_shader);
		}
		if self.pso.is_null()
		{
			return Err(NeHeError::sdl("SDL_CreateGPUGraphicsPipeline"));
		}

		// Create texture sampler
		let mut sampler_info = SDL_GPUSamplerCreateInfo::default();
		sampler_info.min_filter = SDL_GPU_FILTER_LINEAR;
		sampler_info.mag_filter = SDL_GPU_FILTER_LINEAR;
		self.sampler = unsafe { SDL_CreateGPUSampler(ctx.device, &sampler_info).as_mut() }
			.ok_or(NeHeError::sdl("SDL_CreateGPUSampler"))?;

		ctx.copy_pass(|pass|
		{
			self.texture = pass.load_texture("Data/NeHe.bmp", true, false)?;
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
			SDL_ReleaseGPUSampler(ctx.device, self.sampler);
			SDL_ReleaseGPUGraphicsPipeline(ctx.device, self.pso);
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
			SDL_BindGPUGraphicsPipeline(pass, self.pso);

			// Bind texture
			let texture_binding = SDL_GPUTextureSamplerBinding { texture: self.texture, sampler: self.sampler };
			SDL_BindGPUFragmentSamplers(pass, 0, &texture_binding, 1);

			// Bind vertex & index buffers
			let vtx_binding = SDL_GPUBufferBinding { buffer: self.vtx_buffer, offset: 0 };
			let idx_binding = SDL_GPUBufferBinding { buffer: self.idx_buffer, offset: 0 };
			SDL_BindGPUVertexBuffers(pass, 0, &vtx_binding, 1);
			SDL_BindGPUIndexBuffer(pass, &idx_binding, SDL_GPU_INDEXELEMENTSIZE_16BIT);

			// Move cube 5 units into the screen and apply some rotations
			let mut model = Mtx::translation(0.0, 0.0, -5.0);
			model.rotate(self.rot.0, 1.0, 0.0, 0.0);
			model.rotate(self.rot.1, 0.0, 1.0, 0.0);
			model.rotate(self.rot.2, 0.0, 0.0, 1.0);

			// Push shader uniforms
			let model_view_proj = self.projection * model;
			SDL_PushGPUVertexUniformData(cmd, 0,
				addr_of!(model_view_proj) as *const c_void,
				size_of::<Mtx>() as u32);

			// Draw the textured cube
			SDL_DrawGPUIndexedPrimitives(pass, INDICES.len() as u32, 1, 0, 0, 0);

			SDL_EndGPURenderPass(pass);
		}

		self.rot.0 += 0.3;
		self.rot.1 += 0.2;
		self.rot.2 += 0.4;
	}
}

pub fn main() -> Result<ExitCode, Box<dyn std::error::Error>>
{
	run::<Lesson6>()?;
	Ok(ExitCode::SUCCESS)
}
