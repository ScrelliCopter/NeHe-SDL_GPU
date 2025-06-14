/*
 * SPDX-FileCopyrightText: (C) 2025 a dinosaur
 * SPDX-License-Identifier: Zlib
 */

use nehe::application::config::AppImplementation;
use nehe::application::run;
use nehe::context::NeHeContext;
use nehe::error::NeHeError;
use nehe::matrix::Mtx;
use sdl3_sys::everything::SDL_Keycode;
use sdl3_sys::gpu::*;
use sdl3_sys::keyboard::SDL_GetKeyboardState;
use sdl3_sys::keycode::{SDLK_B, SDLK_F};
use sdl3_sys::pixels::SDL_FColor;
use sdl3_sys::scancode::*;
use std::cmp::max;
use std::ffi::c_void;
use std::fs::File;
use std::io::{BufRead, BufReader};
use std::mem::offset_of;
use std::process::ExitCode;
use std::ptr::{addr_of, null_mut};

#[repr(C)]
struct Vertex
{
	x: f32, y: f32, z: f32,
	u: f32, v: f32,
}

#[derive(Default)]
struct Sector { pub vertices: Vec<Vertex> }

impl Sector
{
	fn load_world(&mut self, filename: &str) -> Result<(), NeHeError>
	{
		// Read world file as lines
		let file = File::open(filename)?;
		let mut lines = BufReader::new(file).lines()
			.map(|line| line.unwrap())
			.filter(|line| !line.trim().is_empty() && !line.starts_with("/"));

		// Read the number of triangles
		let first = lines.next()
			.ok_or(NeHeError::Fatal("Empty world text file"))?;
		let num_triangles = first.strip_prefix("NUMPOLLIES ")
				.ok_or(NeHeError::Fatal("World file didn't start with NUMPOLLIES"))?
			.parse::<usize>()
				.map_err(|_| NeHeError::Fatal("Invalid NUMPOLLIES definition"))?;

		// Read remaining lines of "%f %f %f %f %f" as triangle list vertices
		self.vertices = lines.take(3 * num_triangles).map(|line|
		{
			let mut tokens = line.split_whitespace();
			let mut scanf = || tokens.next()
				.map_or(0.0,  |token| token.parse::<f32>()
					.unwrap_or(0.0));
			Vertex
			{
				x: scanf(), y: scanf(), z: scanf(),
				u: scanf(), v: scanf(),
			}
		}).collect();
		Ok(())
	}
}

#[derive(Default)]
struct Camera
{
	x: f32, z: f32,
	yaw: f32, pitch: f32,
	walk_bob: f32, walk_bob_theta: f32,
}

impl Camera
{
	fn update(&mut self, keys: &[bool])
	{
		if keys[SDL_SCANCODE_UP.0 as usize]
		{
			self.x -= (self.yaw * Self::PI_OVER_180).sin() * 0.05;
			self.z -= (self.yaw * Self::PI_OVER_180).cos() * 0.05;
			self.update_walk_bob(10.0)
		}
		if keys[SDL_SCANCODE_DOWN.0 as usize]
		{
			self.x += (self.yaw * Self::PI_OVER_180).sin() * 0.05;
			self.z += (self.yaw * Self::PI_OVER_180).cos() * 0.05;
			self.update_walk_bob(-10.0)
		}
		if keys[SDL_SCANCODE_LEFT.0 as usize]     { self.yaw += 1.0; }
		if keys[SDL_SCANCODE_RIGHT.0 as usize]    { self.yaw -= 1.0; }
		if keys[SDL_SCANCODE_PAGEUP.0 as usize]   { self.pitch -= 1.0; }
		if keys[SDL_SCANCODE_PAGEDOWN.0 as usize] { self.pitch += 1.0; }
	}

	fn position(&self) -> (f32, f32, f32) { (self.x, Self::HEIGHT + self.walk_bob, self.z) }

	fn update_walk_bob(&mut self, delta: f32)
	{
		if delta.is_sign_positive() && self.walk_bob_theta >= 359.0
		{
			self.walk_bob_theta = 0.0;
		}
		else if delta.is_sign_negative() && self.walk_bob_theta <= 1.0
		{
			self.walk_bob_theta = 359.0;
		}
		else
		{
			self.walk_bob_theta += delta;
		}
		self.walk_bob = (self.walk_bob_theta * Self::PI_OVER_180).sin() / 20.0;
	}

	const HEIGHT: f32 = 0.25;
	const PI_OVER_180: f32 = 0.0174532925;
}

struct Lesson10
{
	pso: *mut SDL_GPUGraphicsPipeline,
	pso_blend: *mut SDL_GPUGraphicsPipeline,
	vtx_buffer: *mut SDL_GPUBuffer,
	samplers: [*mut SDL_GPUSampler; 3],
	texture: *mut SDL_GPUTexture,

	projection: Mtx,

	blending: bool,
	filter: usize,

	camera: Camera,
	world: Sector,
}

//FIXME: remove when `raw_ptr_default`
impl Default for Lesson10
{
	fn default() -> Self
	{
		Lesson10
		{
			pso: null_mut(),
			pso_blend: null_mut(),
			vtx_buffer: null_mut(),
			samplers: [null_mut(); 3],
			texture: null_mut(),

			projection: Mtx::IDENTITY,

			blending: false,
			filter: 0,

			camera: Camera::default(),
			world: Sector::default(),
		}
	}
}

impl AppImplementation for Lesson10
{
	const TITLE: &'static str = "Lionel Brits & NeHe's 3D World Tutorial";
	const WIDTH: i32 = 640;
	const HEIGHT: i32 = 480;
	const CREATE_DEPTH_BUFFER: SDL_GPUTextureFormat = SDL_GPU_TEXTUREFORMAT_D16_UNORM;

	fn init(&mut self, ctx: &NeHeContext) -> Result<(), NeHeError>
	{
		self.world.load_world("Data/World.txt")?;

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

		// Common pipeline target info setup
		let mut color_targets = [ SDL_GPUColorTargetDescription::default() ];
		color_targets[0].format = unsafe { SDL_GetGPUSwapchainTextureFormat(ctx.device, ctx.window) };
		pso_info.target_info.color_target_descriptions = color_targets.as_ptr();
		pso_info.target_info.num_color_targets         = color_targets.len() as u32;
		pso_info.target_info.depth_stencil_format      = SDL_GPU_TEXTUREFORMAT_D16_UNORM;
		pso_info.target_info.has_depth_stencil_target  = true;

		// Create blend pipeline (no depth test)
		color_targets[0].blend_state.enable_blend = true;
		color_targets[0].blend_state.color_blend_op = SDL_GPU_BLENDOP_ADD;
		color_targets[0].blend_state.alpha_blend_op = SDL_GPU_BLENDOP_ADD;
		color_targets[0].blend_state.src_color_blendfactor = SDL_GPU_BLENDFACTOR_SRC_ALPHA;
		color_targets[0].blend_state.dst_color_blendfactor = SDL_GPU_BLENDFACTOR_ONE;
		color_targets[0].blend_state.src_alpha_blendfactor = SDL_GPU_BLENDFACTOR_SRC_ALPHA;
		color_targets[0].blend_state.dst_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE;
		self.pso_blend = unsafe { SDL_CreateGPUGraphicsPipeline(ctx.device, &pso_info).as_mut() }
			.ok_or(NeHeError::sdl("SDL_CreateGPUGraphicsPipeline"))?;

		// Create regular pipeline w/ depth testing
		pso_info.depth_stencil_state.compare_op         = SDL_GPU_COMPAREOP_LESS;
		pso_info.depth_stencil_state.enable_depth_test  = true;
		pso_info.depth_stencil_state.enable_depth_write = true;
		color_targets[0].blend_state = SDL_GPUColorTargetBlendState::default();
		self.pso = unsafe { SDL_CreateGPUGraphicsPipeline(ctx.device, &pso_info).as_mut() }
			.ok_or(NeHeError::sdl("SDL_CreateGPUGraphicsPipeline"))?;

		// Create texture samplers (nearest, linear, and trilinear mipmap)
		let create_sampler = |filter: SDL_GPUFilter, enable_mip: bool|
			-> Result<&mut SDL_GPUSampler, NeHeError>
		{
			let mut sampler_info = SDL_GPUSamplerCreateInfo::default();
			sampler_info.min_filter = filter;
			sampler_info.mag_filter = filter;
			if enable_mip
			{
				sampler_info.mipmap_mode = SDL_GPU_SAMPLERMIPMAPMODE_LINEAR;
				sampler_info.max_lod = f32::MAX;
			}
			unsafe { SDL_CreateGPUSampler(ctx.device, &sampler_info).as_mut() }
				.ok_or(NeHeError::sdl("SDL_CreateGPUSampler"))
		};
		self.samplers[0] = create_sampler(SDL_GPU_FILTER_NEAREST, false)?;
		self.samplers[1] = create_sampler(SDL_GPU_FILTER_LINEAR, false)?;
		self.samplers[2] = create_sampler(SDL_GPU_FILTER_LINEAR, true)?;

		// Upload texture and world vertex data
		ctx.copy_pass(|pass|
		{
			self.texture = pass.load_texture("Data/Mud.bmp", true, true)?;
			self.vtx_buffer = pass.create_buffer(SDL_GPU_BUFFERUSAGE_VERTEX, &*self.world.vertices)?;
			Ok(())
		})
	}

	fn quit(&mut self, ctx: &NeHeContext)
	{
		unsafe
		{
			SDL_ReleaseGPUBuffer(ctx.device, self.vtx_buffer);
			SDL_ReleaseGPUTexture(ctx.device, self.texture);
			self.samplers.iter().rev().for_each(|sampler| SDL_ReleaseGPUSampler(ctx.device, *sampler));
			SDL_ReleaseGPUGraphicsPipeline(ctx.device, self.pso);
			SDL_ReleaseGPUGraphicsPipeline(ctx.device, self.pso_blend);
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
		color_info.clear_color = SDL_FColor { r: 0.0, g: 0.0, b: 0.0, a: 0.0 };
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
			SDL_BindGPUGraphicsPipeline(pass, if self.blending { self.pso_blend } else { self.pso });

			// Bind texture
			let texture_binding = SDL_GPUTextureSamplerBinding { texture: self.texture, sampler: self.samplers[self.filter] };
			SDL_BindGPUFragmentSamplers(pass, 0, &texture_binding, 1);

			// Bind world mesh vertex buffer
			let vtx_binding = SDL_GPUBufferBinding { buffer: self.vtx_buffer, offset: 0 };
			SDL_BindGPUVertexBuffers(pass, 0, &vtx_binding, 1);

			// Setup the camera view matrix
			let mut model_view = Mtx::rotation(self.camera.pitch, 1.0, 0.0, 0.0);
			model_view.rotate(360.0 - self.camera.yaw, 0.0, 1.0, 0.0);
			let (cam_x, cam_y, cam_z) = self.camera.position();
			model_view.translate(-cam_x, -cam_y, -cam_z);

			// Push shader uniforms
			let model_view_proj = self.projection * model_view;
			SDL_PushGPUVertexUniformData(cmd, 0,
				addr_of!(model_view_proj) as *const c_void,
				size_of::<Mtx>() as u32);

			// Draw world
			SDL_DrawGPUPrimitives(pass, self.world.vertices.len() as u32, 1, 0, 0);

			SDL_EndGPURenderPass(pass);
		}

		// Handle keyboard input
		let keys = unsafe
		{
			let mut numkeys: std::ffi::c_int = 0;
			let keys = SDL_GetKeyboardState(&mut numkeys);
			std::slice::from_raw_parts(keys, numkeys as usize)
		};
		self.camera.update(keys);
	}

	fn key(&mut self, _ctx: &NeHeContext, key: SDL_Keycode, down: bool, repeat: bool)
	{
		match key
		{
			SDLK_B if down && !repeat => self.blending = !self.blending,
			SDLK_F if down => self.filter = (self.filter + 1) % self.samplers.len(),
			_ => ()
		}
	}
}

pub fn main() -> Result<ExitCode, Box<dyn std::error::Error>>
{
	run::<Lesson10>()?;
	Ok(ExitCode::SUCCESS)
}
