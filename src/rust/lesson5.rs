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
use std::ptr::null_mut;

#[repr(C)]
struct Vertex
{
	x: f32, y: f32, z: f32,
	r: f32, g: f32, b: f32, a: f32,
}

const VERTICES: &'static [Vertex] =
&[
	// Pyramid
	Vertex { x:  0.0, y:  1.0, z:  0.0, r: 1.0, g: 0.0, b: 0.0, a: 1.0 },  // Top of pyramid (Red)
	Vertex { x: -1.0, y: -1.0, z:  1.0, r: 0.0, g: 1.0, b: 0.0, a: 1.0 },  // Front-left of pyramid (Green)
	Vertex { x:  1.0, y: -1.0, z:  1.0, r: 0.0, g: 0.0, b: 1.0, a: 1.0 },  // Front-right of pyramid (Blue)
	Vertex { x:  1.0, y: -1.0, z: -1.0, r: 0.0, g: 1.0, b: 0.0, a: 1.0 },  // Back-right of pyramid (Green)
	Vertex { x: -1.0, y: -1.0, z: -1.0, r: 0.0, g: 0.0, b: 1.0, a: 1.0 },  // Back-left of pyramid (Blue)
	// Cube
	Vertex { x:  1.0, y:  1.0, z: -1.0, r: 0.0, g: 1.0, b: 0.0, a: 1.0 },  // Top-right of top face (Green)
	Vertex { x: -1.0, y:  1.0, z: -1.0, r: 0.0, g: 1.0, b: 0.0, a: 1.0 },  // Top-left of top face (Green)
	Vertex { x: -1.0, y:  1.0, z:  1.0, r: 0.0, g: 1.0, b: 0.0, a: 1.0 },  // Bottom-left of top face (Green)
	Vertex { x:  1.0, y:  1.0, z:  1.0, r: 0.0, g: 1.0, b: 0.0, a: 1.0 },  // Bottom-right of top face (Green)
	Vertex { x:  1.0, y: -1.0, z:  1.0, r: 1.0, g: 0.5, b: 0.0, a: 1.0 },  // Top-right of bottom face (Orange)
	Vertex { x: -1.0, y: -1.0, z:  1.0, r: 1.0, g: 0.5, b: 0.0, a: 1.0 },  // Top-left of bottom face (Orange)
	Vertex { x: -1.0, y: -1.0, z: -1.0, r: 1.0, g: 0.5, b: 0.0, a: 1.0 },  // Bottom-left of bottom face (Orange)
	Vertex { x:  1.0, y: -1.0, z: -1.0, r: 1.0, g: 0.5, b: 0.0, a: 1.0 },  // Bottom-right of bottom face (Orange)
	Vertex { x:  1.0, y:  1.0, z:  1.0, r: 1.0, g: 0.0, b: 0.0, a: 1.0 },  // Top-right of front face (Red)
	Vertex { x: -1.0, y:  1.0, z:  1.0, r: 1.0, g: 0.0, b: 0.0, a: 1.0 },  // Top-left of front face (Red)
	Vertex { x: -1.0, y: -1.0, z:  1.0, r: 1.0, g: 0.0, b: 0.0, a: 1.0 },  // Bottom-left of front face (Red)
	Vertex { x:  1.0, y: -1.0, z:  1.0, r: 1.0, g: 0.0, b: 0.0, a: 1.0 },  // Bottom-right of front face (Red)
	Vertex { x:  1.0, y: -1.0, z: -1.0, r: 1.0, g: 1.0, b: 0.0, a: 1.0 },  // Top-right of back face (Yellow)
	Vertex { x: -1.0, y: -1.0, z: -1.0, r: 1.0, g: 1.0, b: 0.0, a: 1.0 },  // Top-left of back face (Yellow)
	Vertex { x: -1.0, y:  1.0, z: -1.0, r: 1.0, g: 1.0, b: 0.0, a: 1.0 },  // Bottom-left of back face (Yellow)
	Vertex { x:  1.0, y:  1.0, z: -1.0, r: 1.0, g: 1.0, b: 0.0, a: 1.0 },  // Bottom-right of back face (Yellow)
	Vertex { x: -1.0, y:  1.0, z:  1.0, r: 0.0, g: 0.0, b: 1.0, a: 1.0 },  // Top-right of left face (Blue)
	Vertex { x: -1.0, y:  1.0, z: -1.0, r: 0.0, g: 0.0, b: 1.0, a: 1.0 },  // Top-left of left face (Blue)
	Vertex { x: -1.0, y: -1.0, z: -1.0, r: 0.0, g: 0.0, b: 1.0, a: 1.0 },  // Bottom-left of left face (Blue)
	Vertex { x: -1.0, y: -1.0, z:  1.0, r: 0.0, g: 0.0, b: 1.0, a: 1.0 },  // Bottom-right of left face (Blue)
	Vertex { x:  1.0, y:  1.0, z: -1.0, r: 1.0, g: 0.0, b: 1.0, a: 1.0 },  // Top-right of right face (Violet)
	Vertex { x:  1.0, y:  1.0, z:  1.0, r: 1.0, g: 0.0, b: 1.0, a: 1.0 },  // Top-left of right face (Violet)
	Vertex { x:  1.0, y: -1.0, z:  1.0, r: 1.0, g: 0.0, b: 1.0, a: 1.0 },  // Bottom-left of right face (Violet)
	Vertex { x:  1.0, y: -1.0, z: -1.0, r: 1.0, g: 0.0, b: 1.0, a: 1.0 },  // Bottom-right of right face (Violet)
];

const INDICES: &'static [u16] =
&[
	// Pyramid
	0, 1, 2,  // Front
	0, 2, 3,  // Right
	0, 3, 4,  // Back
	0, 4, 1,  // Left
	// Cube
	 5,  6,  7,   7,  8,  5,  // Top
	 9, 10, 11,  11, 12,  9,  // Bottom
	13, 14, 15,  15, 16, 13,  // Front
	17, 18, 19,  19, 20, 17,  // Back
	21, 22, 23,  23, 24, 21,  // Left
	25, 26, 27,  27, 28, 25,  // Right
];

struct Lesson5
{
	pso: *mut SDL_GPUGraphicsPipeline,
	vtx_buffer: *mut SDL_GPUBuffer,
	idx_buffer: *mut SDL_GPUBuffer,
	projection: Mtx,

	rot_tri: f32,
	rot_quad: f32,
}

//FIXME: remove when `raw_ptr_default`
impl Default for Lesson5
{
	fn default() -> Self
	{
		Self
		{
			pso: null_mut(),
			vtx_buffer: null_mut(),
			idx_buffer: null_mut(),
			projection: Mtx::IDENTITY,

			rot_tri: 0.0,
			rot_quad: 0.0,
		}
	}
}

impl AppImplementation for Lesson5
{
	const TITLE: &'static str = "NeHe's Solid Object Tutorial";
	const WIDTH: i32 = 640;
	const HEIGHT: i32 = 480;
	const CREATE_DEPTH_BUFFER: SDL_GPUTextureFormat = SDL_GPU_TEXTUREFORMAT_D16_UNORM;

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

		let mut info = SDL_GPUGraphicsPipelineCreateInfo::default();
		info.vertex_shader   = vertex_shader;
		info.fragment_shader = fragment_shader;
		info.primitive_type  = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;
		info.vertex_input_state = SDL_GPUVertexInputState
		{
			vertex_buffer_descriptions: VERTEX_DESCRIPTIONS.as_ptr(),
			num_vertex_buffers: VERTEX_DESCRIPTIONS.len() as u32,
			vertex_attributes: VERTEX_ATTRIBS.as_ptr(),
			num_vertex_attributes: VERTEX_ATTRIBS.len() as u32,
		};
		info.rasterizer_state.fill_mode  = SDL_GPU_FILLMODE_FILL;
		info.rasterizer_state.cull_mode  = SDL_GPU_CULLMODE_NONE;
		info.rasterizer_state.front_face = SDL_GPU_FRONTFACE_COUNTER_CLOCKWISE;
		let colour_targets: &[SDL_GPUColorTargetDescription] =
		&[
			SDL_GPUColorTargetDescription
			{
				format: unsafe { SDL_GetGPUSwapchainTextureFormat(ctx.device, ctx.window) },
				blend_state: SDL_GPUColorTargetBlendState::default(),
			}
		];
		info.target_info.color_target_descriptions = colour_targets.as_ptr();
		info.target_info.num_color_targets         = colour_targets.len() as u32;
		info.target_info.depth_stencil_format      = SDL_GPU_TEXTUREFORMAT_D16_UNORM;
		info.target_info.has_depth_stencil_target  = true;

		info.depth_stencil_state.compare_op         = SDL_GPU_COMPAREOP_LESS_OR_EQUAL;
		info.depth_stencil_state.enable_depth_test  = true;
		info.depth_stencil_state.enable_depth_write = true;

		self.pso = unsafe { SDL_CreateGPUGraphicsPipeline(ctx.device, &info) };
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

			// Bind vertex & index buffers
			let vtx_binding = SDL_GPUBufferBinding { buffer: self.vtx_buffer, offset: 0 };
			let idx_binding = SDL_GPUBufferBinding { buffer: self.idx_buffer, offset: 0 };
			SDL_BindGPUVertexBuffers(pass, 0, &vtx_binding, 1);
			SDL_BindGPUIndexBuffer(pass, &idx_binding, SDL_GPU_INDEXELEMENTSIZE_16BIT);

			// Draw pyramid 1.5 units to the left and 6 units into the camera
			let mut model = Mtx::translation(-1.5, 0.0, -6.0);
			model.rotate(self.rot_tri, 0.0, 1.0, 0.0);
			let mut viewproj = self.projection * model;
			SDL_PushGPUVertexUniformData(cmd, 0, viewproj.as_ptr() as *const c_void, size_of::<Mtx>() as u32);
			SDL_DrawGPUIndexedPrimitives(pass, 12, 1, 0, 0, 0);

			// Draw cube 1.5 units to the right and 7 units into the camera
			model = Mtx::translation(1.5, 0.0, -7.0);
			model.rotate(self.rot_quad, 1.0, 1.0, 1.0);
			viewproj = self.projection * model;
			SDL_PushGPUVertexUniformData(cmd, 0, viewproj.as_ptr() as *const c_void, size_of::<Mtx>() as u32);
			SDL_DrawGPUIndexedPrimitives(pass, 36, 1, 12, 0, 0);

			SDL_EndGPURenderPass(pass);
		}

		self.rot_tri += 0.2;
		self.rot_quad -= 0.15;
	}
}

pub fn main() -> Result<ExitCode, Box<dyn std::error::Error>>
{
	run::<Lesson5>()?;
	Ok(ExitCode::SUCCESS)
}
