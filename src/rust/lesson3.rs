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
use std::ptr::{null, null_mut};

#[repr(C)]
struct Vertex
{
	x: f32, y: f32, z: f32,
	r: f32, g: f32, b: f32, a: f32,
}

const VERTICES: &'static [Vertex] =
&[
	// Triangle
	Vertex { x:  0.0, y:  1.0, z: 0.0, r: 1.0, g: 0.0, b: 0.0, a: 1.0 },  // Top (red)
	Vertex { x: -1.0, y: -1.0, z: 0.0, r: 0.0, g: 1.0, b: 0.0, a: 1.0 },  // Bottom left (green)
	Vertex { x:  1.0, y: -1.0, z: 0.0, r: 0.0, g: 0.0, b: 1.0, a: 1.0 },  // Bottom right (blue)
	// Quad
	Vertex { x: -1.0, y:  1.0, z: 0.0, r: 0.5, g: 0.5, b: 1.0, a: 1.0 },  // Top left
	Vertex { x:  1.0, y:  1.0, z: 0.0, r: 0.5, g: 0.5, b: 1.0, a: 1.0 },  // Top right
	Vertex { x:  1.0, y: -1.0, z: 0.0, r: 0.5, g: 0.5, b: 1.0, a: 1.0 },  // Bottom right
	Vertex { x: -1.0, y: -1.0, z: 0.0, r: 0.5, g: 0.5, b: 1.0, a: 1.0 },  // Bottom left
];

const INDICES: &'static [i16] =
&[
	// Triangle
	0, 1, 2,
	// Quad
	3, 4, 5, 5, 6, 3,
];

struct Lesson3
{
	pso: *mut SDL_GPUGraphicsPipeline,
	vtx_buffer: *mut SDL_GPUBuffer,
	idx_buffer: *mut SDL_GPUBuffer,
	projection: Mtx,
}

//FIXME: remove when `raw_ptr_default`
impl Default for Lesson3
{
	fn default() -> Self
	{
		Self
		{
			pso: null_mut(),
			vtx_buffer: null_mut(),
			idx_buffer: null_mut(),
			projection: Mtx::IDENTITY,
		}
	}
}

impl AppImplementation for Lesson3
{
	const TITLE: &'static str = "NeHe's Color Tutorial";
	const WIDTH: i32 = 640;
	const HEIGHT: i32 = 480;

	fn init(&mut self, ctx: &NeHeContext) -> Result<(), NeHeError>
	{
		let (vertex_shader, fragment_shader) = ctx.load_shaders("lesson3", 1, 0, 0)?;

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
				format: SDL_GPU_VERTEXELEMENTFORMAT_FLOAT4,
				offset: offset_of!(Vertex, r) as u32,
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
		pso_info.target_info.num_color_targets = color_targets.len() as u32;

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

		ctx.copy_pass(|pass|
		{
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
			SDL_ReleaseGPUGraphicsPipeline(ctx.device, self.pso);
		}
	}

	fn resize(&mut self, _ctx: &NeHeContext, width: i32, height: i32)
	{
		let aspect = width as f32 / max(height, 1) as f32;
		self.projection = Mtx::perspective(45.0, aspect, 0.1, 100.0);
	}

	fn draw(&mut self, _ctx: &NeHeContext, cmd: *mut SDL_GPUCommandBuffer, swapchain: *mut SDL_GPUTexture)
	{
		let mut color_info = SDL_GPUColorTargetInfo::default();
		color_info.texture     = swapchain;
		color_info.clear_color = SDL_FColor { r: 0.0, g: 0.0, b: 0.0, a: 0.5 };
		color_info.load_op     = SDL_GPU_LOADOP_CLEAR;
		color_info.store_op    = SDL_GPU_STOREOP_STORE;

		unsafe
		{
			// Begin pass & bind pipeline state
			let pass = SDL_BeginGPURenderPass(cmd, &color_info, 1, null());
			SDL_BindGPUGraphicsPipeline(pass, self.pso);

			// Bind vertex & index buffers
			let vtx_binding = SDL_GPUBufferBinding { buffer: self.vtx_buffer, offset: 0 };
			let idx_binding = SDL_GPUBufferBinding { buffer: self.idx_buffer, offset: 0 };
			SDL_BindGPUVertexBuffers(pass, 0, &vtx_binding, 1);
			SDL_BindGPUIndexBuffer(pass, &idx_binding, SDL_GPU_INDEXELEMENTSIZE_16BIT);

			// Draw triangle 1.5 units to the left and 6 units into the camera
			let mut model = Mtx::translation(-1.5, 0.0, -6.0);
			let mut viewproj = self.projection * model;
			SDL_PushGPUVertexUniformData(cmd, 0, viewproj.as_ptr() as *const c_void, size_of::<Mtx>() as u32);
			SDL_DrawGPUIndexedPrimitives(pass, 3, 1, 0, 0, 0);

			// Move to the right by 3 units and draw quad
			model.translate(3.0, 0.0, 0.0);
			viewproj = self.projection * model;
			SDL_PushGPUVertexUniformData(cmd, 0, viewproj.as_ptr() as *const c_void, size_of::<Mtx>() as u32);
			SDL_DrawGPUIndexedPrimitives(pass, 6, 1, 3, 0, 0);

			SDL_EndGPURenderPass(pass);
		}
	}
}

pub fn main() -> Result<ExitCode, Box<dyn std::error::Error>>
{
	run::<Lesson3>()?;
	Ok(ExitCode::SUCCESS)
}
