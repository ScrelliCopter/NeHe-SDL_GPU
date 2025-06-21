/*
 * SPDX-FileCopyrightText: (C) 2025 a dinosaur
 * SPDX-License-Identifier: Zlib
 */

use nehe::application::config::AppImplementation;
use nehe::application::run;
use nehe::context::NeHeContext;
use nehe::error::NeHeError;
use nehe::matrix::Mtx;
use nehe::random::Random;
use sdl3_sys::gpu::*;
use sdl3_sys::keyboard::SDL_GetKeyboardState;
use sdl3_sys::keycode::{SDL_Keycode, SDLK_T};
use sdl3_sys::pixels::SDL_FColor;
use sdl3_sys::scancode::*;
use std::cmp::max;
use std::ffi::c_void;
use std::mem::offset_of;
use std::process::ExitCode;
use std::ptr::{null, null_mut};

#[repr(C)]
struct Vertex
{
	x: f32, y: f32, z: f32,
	u: f32, v: f32,
}

#[repr(C)]
struct Instance
{
	model: Mtx,
	color: [f32; 4],
}

#[derive(Default, Copy, Clone)]
struct Star
{
	angle: f32,
	distance: f32,
	color: [u8; 3],
}

impl Star
{
	fn new(coeff: f32, random: &mut Random) -> Star
	{
		Star
		{
			angle: 0.0,
			distance: 5.0 * coeff,
			color: Self::next_color(random),
		}
	}

	fn update(&mut self, coeff: f32, random: &mut Random)
	{
		self.angle += coeff;
		self.distance -= 0.01;
		if self.distance < 0.0
		{
			self.distance += 5.0;
			self.color = Self::next_color(random);
		}
	}

	fn color_rgba_f32(&self) -> [f32; 4]
	{
		[
			self.color[0] as f32 / 255.0,
			self.color[1] as f32 / 255.0,
			self.color[2] as f32 / 255.0,
			1.0,
		]
	}

	fn next_color(random: &mut Random) -> [u8; 3]
	{
		[
			(random.next() % 256) as u8,
			(random.next() % 256) as u8,
			(random.next() % 256) as u8,
		]
	}

	fn coeff(star_id: usize, num_stars: usize) -> f32 { star_id as f32 / num_stars as f32 }
}

const VERTICES: &'static [Vertex] =
&[
	Vertex { x: -1.0, y: -1.0, z: 0.0, u: 0.0, v: 0.0 },
	Vertex { x:  1.0, y: -1.0, z: 0.0, u: 1.0, v: 0.0 },
	Vertex { x:  1.0, y:  1.0, z: 0.0, u: 1.0, v: 1.0 },
	Vertex { x: -1.0, y:  1.0, z: 0.0, u: 0.0, v: 1.0 },
];

const INDICES: &'static [u16] =
&[
	0,  1,  2,
	2,  3,  0,
];

struct Lesson9
{
	pso: *mut SDL_GPUGraphicsPipeline,
	vtx_buffer: *mut SDL_GPUBuffer,
	idx_buffer: *mut SDL_GPUBuffer,
	instance_buffer: *mut SDL_GPUBuffer,
	instance_xfer_buffer: *mut SDL_GPUTransferBuffer,
	sampler: *mut SDL_GPUSampler,
	texture: *mut SDL_GPUTexture,

	projection: Mtx,

	twinkle: bool,
	stars: [Star; 50],

	zoom: f32,
	tilt: f32,
	spin: f32,

	random: Random,
}

impl Default for Lesson9
{
	fn default() -> Self
	{
		Self
		{
			pso: null_mut(),
			vtx_buffer: null_mut(),
			idx_buffer: null_mut(),
			instance_buffer: null_mut(),
			instance_xfer_buffer: null_mut(),
			sampler: null_mut(),
			texture: null_mut(),

			projection: Mtx::IDENTITY,

			twinkle: false,
			stars: [Star::default(); 50],

			zoom: -15.0,
			tilt: 90.0,
			spin: 0.0,

			random: Random::default(),
		}
	}
}

impl AppImplementation for Lesson9
{
	const TITLE: &'static str = "NeHe's Animated Blended Textures Tutorial";
	const WIDTH: i32 = 640;
	const HEIGHT: i32 = 480;

	fn init(&mut self, ctx: &NeHeContext) -> Result<(), NeHeError>
	{
		let (vertex_shader, fragment_shader) = ctx.load_shaders("lesson9", 1, 0, 1)?;

		const VERTEX_DESCRIPTIONS: &'static [SDL_GPUVertexBufferDescription] =
		&[
			// Slot for mesh
			SDL_GPUVertexBufferDescription
			{
				slot: 0,
				pitch: size_of::<Vertex>() as u32,
				input_rate: SDL_GPU_VERTEXINPUTRATE_VERTEX,
				instance_step_rate: 0,
			},
			// Slot for instances
			SDL_GPUVertexBufferDescription
			{
				slot: 1,
				pitch: size_of::<Instance>() as u32,
				input_rate: SDL_GPU_VERTEXINPUTRATE_INSTANCE,
				instance_step_rate: 0,
			},
		];
		const VERTEX_ATTRIBS: &'static [SDL_GPUVertexAttribute] =
		&[
			// Mesh attributes
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
			// Instance matrix attributes (one for each column)
			SDL_GPUVertexAttribute
			{
				location: 2,
				buffer_slot: 1,
				format: SDL_GPU_VERTEXELEMENTFORMAT_FLOAT4,
				offset: offset_of!(Instance, model) as u32,
			},
			SDL_GPUVertexAttribute
			{
				location: 3,
				buffer_slot: 1,
				format: SDL_GPU_VERTEXELEMENTFORMAT_FLOAT4,
				offset: offset_of!(Instance, model) as u32 + 16,
			},
			SDL_GPUVertexAttribute
			{
				location: 4,
				buffer_slot: 1,
				format: SDL_GPU_VERTEXELEMENTFORMAT_FLOAT4,
				offset: offset_of!(Instance, model) as u32 + 32,
			},
			SDL_GPUVertexAttribute
			{
				location: 5,
				buffer_slot: 1,
				format: SDL_GPU_VERTEXELEMENTFORMAT_FLOAT4,
				offset: offset_of!(Instance, model) as u32 + 48,
			},
			// Instance colour
			SDL_GPUVertexAttribute
			{
				location: 6,
				buffer_slot: 1,
				format: SDL_GPU_VERTEXELEMENTFORMAT_FLOAT4,
				offset: offset_of!(Instance, color) as u32,
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

		let mut color_targets = [ SDL_GPUColorTargetDescription::default() ];
		color_targets[0].format = unsafe { SDL_GetGPUSwapchainTextureFormat(ctx.device, ctx.window) };
		color_targets[0].blend_state.enable_blend = true;
		color_targets[0].blend_state.color_blend_op = SDL_GPU_BLENDOP_ADD;
		color_targets[0].blend_state.alpha_blend_op = SDL_GPU_BLENDOP_ADD;
		color_targets[0].blend_state.src_color_blendfactor = SDL_GPU_BLENDFACTOR_SRC_ALPHA;
		color_targets[0].blend_state.dst_color_blendfactor = SDL_GPU_BLENDFACTOR_ONE;
		color_targets[0].blend_state.src_alpha_blendfactor = SDL_GPU_BLENDFACTOR_SRC_ALPHA;
		color_targets[0].blend_state.dst_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE;
		pso_info.target_info.color_target_descriptions = color_targets.as_ptr();
		pso_info.target_info.num_color_targets = color_targets.len() as u32;

		self.pso = unsafe { SDL_CreateGPUGraphicsPipeline(ctx.device, &pso_info).as_mut() }
			.ok_or(NeHeError::sdl("SDL_CreateGPUGraphicsPipeline"))?;

		// Create texture sampler
		let mut sampler_info = SDL_GPUSamplerCreateInfo::default();
		sampler_info.min_filter = SDL_GPU_FILTER_LINEAR;
		sampler_info.mag_filter = SDL_GPU_FILTER_LINEAR;
		self.sampler = unsafe { SDL_CreateGPUSampler(ctx.device, &sampler_info).as_mut() }
			.ok_or(NeHeError::sdl("SDL_CreateGPUSampler"))?;

		ctx.copy_pass(|pass|
		{
			self.texture = pass.load_texture("Data/Star.bmp", true, false)?;
			self.vtx_buffer = pass.create_buffer(SDL_GPU_BUFFERUSAGE_VERTEX, VERTICES)?;
			self.idx_buffer = pass.create_buffer(SDL_GPU_BUFFERUSAGE_INDEX, INDICES)?;
			Ok(())
		})?;

		let num_stars = self.stars.len();

		// Create GPU side buffer for star instances
		let instance_buffer_size = (size_of::<Instance>() * 2 * num_stars) as u32;
		let instance_info = SDL_GPUBufferCreateInfo
		{
			usage: SDL_GPU_BUFFERUSAGE_VERTEX,
			size: instance_buffer_size,
			props: 0,
		};
		self.instance_buffer = unsafe { SDL_CreateGPUBuffer(ctx.device, &instance_info).as_mut() }
			.ok_or(NeHeError::sdl("SDL_CreateGPUBuffer"))?;

		// Create CPU side buffer for star instances (to upload to GPU)
		let instance_xfer_info = SDL_GPUTransferBufferCreateInfo
		{
			usage: SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
			size: instance_buffer_size,
			props: 0,
		};
		self.instance_xfer_buffer = unsafe { SDL_CreateGPUTransferBuffer(ctx.device, &instance_xfer_info).as_mut() }
			.ok_or(NeHeError::sdl("SDL_CreateGPUTransferBuffer"))?;

		// Initialise stars
		for i in 0..num_stars
		{
			let _ = std::mem::replace(&mut self.stars[i],
				Star::new(Star::coeff(i, num_stars), &mut self.random));
		}

		Ok(())
	}

	fn quit(&mut self, ctx: &NeHeContext)
	{
		unsafe
		{
			SDL_ReleaseGPUTransferBuffer(ctx.device, self.instance_xfer_buffer);
			SDL_ReleaseGPUBuffer(ctx.device, self.instance_buffer);
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
		// Animate stars
		let num_stars = self.stars.len();
		let num_instances = if self.twinkle { 2 * num_stars } else { num_stars };
		let instances = unsafe
		{
			let map = SDL_MapGPUTransferBuffer(ctx.device, self.instance_xfer_buffer, true);
			assert!(!map.is_null(), "Failed to map instance transfer buffer");
			std::slice::from_raw_parts_mut(map as *mut Instance, num_instances)
		};
		let mut instance_idx = 0;
		for star_idx in 0..num_stars
		{
			let mut star = self.stars[star_idx];

			let mut model = Mtx::translation(0.0, 0.0, self.zoom);
			model.rotate(  self.tilt, 1.0, 0.0, 0.0);
			model.rotate( star.angle, 0.0, 1.0, 0.0);
			model.translate(star.distance, 0.0, 0.0);
			model.rotate(-star.angle, 0.0, 1.0, 0.0);
			model.rotate( -self.tilt, 1.0, 0.0, 0.0);

			if self.twinkle
			{
				let twinkle_color = self.stars[num_stars - star_idx - 1].color_rgba_f32();
				instances[instance_idx] = Instance { model, color: twinkle_color };
				instance_idx += 1;
			}

			model.rotate(self.spin, 0.0, 0.0, 1.0);
			instances[instance_idx] = Instance { model, color: star.color_rgba_f32() };
			instance_idx += 1;

			self.spin += 0.01;

			// Update star
			star.update(Star::coeff(star_idx, num_stars), &mut self.random);
			let _ = std::mem::replace(&mut self.stars[star_idx], star);
		}
		unsafe { SDL_UnmapGPUTransferBuffer(ctx.device, self.instance_xfer_buffer); }

		// Upload instances buffer to the GPU
		unsafe
		{
			let copy_pass = SDL_BeginGPUCopyPass(cmd);
			let source = SDL_GPUTransferBufferLocation
			{
				transfer_buffer: self.instance_xfer_buffer,
				offset: 0,
			};
			let destination = SDL_GPUBufferRegion
			{
				buffer: self.instance_buffer,
				offset: 0,
				size: (size_of::<Instance>() * num_instances) as u32,
			};
			SDL_UploadToGPUBuffer(copy_pass, &source, &destination, true);
			SDL_EndGPUCopyPass(copy_pass);
		}

		// Begin pass & bind pipeline state
		let mut color_info = SDL_GPUColorTargetInfo::default();
		color_info.texture     = swapchain;
		color_info.clear_color = SDL_FColor { r: 0.0, g: 0.0, b: 0.0, a: 0.5 };
		color_info.load_op     = SDL_GPU_LOADOP_CLEAR;
		color_info.store_op    = SDL_GPU_STOREOP_STORE;
		unsafe
		{
			let render_pass = SDL_BeginGPURenderPass(cmd, &color_info, 1, null());
			SDL_BindGPUGraphicsPipeline(render_pass, self.pso);

			// Bind particle texture
			let texture_binding = SDL_GPUTextureSamplerBinding { texture: self.texture, sampler: self.sampler };
			SDL_BindGPUFragmentSamplers(render_pass, 0, &texture_binding, 1);

			// Bind vertex, instance, and index buffers
			let vtx_bindings =
			[
				SDL_GPUBufferBinding { buffer: self.vtx_buffer, offset: 0 },
				SDL_GPUBufferBinding { buffer: self.instance_buffer, offset: 0 },
			];
			let idx_binding = SDL_GPUBufferBinding { buffer: self.idx_buffer, offset: 0 };
			SDL_BindGPUVertexBuffers(render_pass, 0, vtx_bindings.as_ptr(), vtx_bindings.len() as u32);
			SDL_BindGPUIndexBuffer(render_pass, &idx_binding, SDL_GPU_INDEXELEMENTSIZE_16BIT);

			// Push shader uniforms
			SDL_PushGPUVertexUniformData(cmd, 0, self.projection.as_ptr() as *mut c_void, size_of::<Mtx>() as u32);

			// Draw star instances
			SDL_DrawGPUIndexedPrimitives(render_pass, INDICES.len() as u32, num_instances as u32, 0, 0, 0);

			SDL_EndGPURenderPass(render_pass);
		}

		let keys = unsafe
		{
			let mut numkeys: std::ffi::c_int = 0;
			let keys = SDL_GetKeyboardState(&mut numkeys);
			std::slice::from_raw_parts(keys, numkeys as usize)
		};

		if keys[SDL_SCANCODE_UP.0 as usize]       { self.tilt -= 0.5 }
		if keys[SDL_SCANCODE_DOWN.0 as usize]     { self.tilt += 0.5 }
		if keys[SDL_SCANCODE_PAGEUP.0 as usize]   { self.zoom -= 0.2 }
		if keys[SDL_SCANCODE_PAGEDOWN.0 as usize] { self.zoom += 0.2 }
	}

	fn key(&mut self, _ctx: &NeHeContext, key: SDL_Keycode, down: bool, repeat: bool)
	{
		match key
		{
			SDLK_T if down && !repeat => self.twinkle = !self.twinkle,
			_ => ()
		}
	}
}

pub fn main() -> Result<ExitCode, Box<dyn std::error::Error>>
{
	run::<Lesson9>()?;
	Ok(ExitCode::SUCCESS)
}
