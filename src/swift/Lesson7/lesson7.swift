/*
 * SPDX-FileCopyrightText: (C) 2025 a dinosaur
 * SPDX-License-Identifier: Zlib
 */

import Foundation
import SDLSwift
import NeHe
import simd

struct Lesson7: AppDelegate
{
	struct Vertex
	{
		let position: SIMD3<Float>, normal: SIMD3<Float>, texcoord: SIMD2<Float>

		init(_ position: SIMD3<Float>, _ normal: SIMD3<Float>, _ texcoord: SIMD2<Float>)
		{
			self.position = position
			self.normal   = normal
			self.texcoord = texcoord
		}
	}

	static let vertices =
	[
		// Front Face
		Vertex(.init(-1.0, -1.0,  1.0), .init( 0.0,  0.0,  1.0), .init(0.0, 0.0)),
		Vertex(.init( 1.0, -1.0,  1.0), .init( 0.0,  0.0,  1.0), .init(1.0, 0.0)),
		Vertex(.init( 1.0,  1.0,  1.0), .init( 0.0,  0.0,  1.0), .init(1.0, 1.0)),
		Vertex(.init(-1.0,  1.0,  1.0), .init( 0.0,  0.0,  1.0), .init(0.0, 1.0)),
		// Back Face
		Vertex(.init(-1.0, -1.0, -1.0), .init( 0.0,  0.0, -1.0), .init(1.0, 0.0)),
		Vertex(.init(-1.0,  1.0, -1.0), .init( 0.0,  0.0, -1.0), .init(1.0, 1.0)),
		Vertex(.init( 1.0,  1.0, -1.0), .init( 0.0,  0.0, -1.0), .init(0.0, 1.0)),
		Vertex(.init( 1.0, -1.0, -1.0), .init( 0.0,  0.0, -1.0), .init(0.0, 0.0)),
		// Top Face
		Vertex(.init(-1.0,  1.0, -1.0), .init( 0.0,  1.0,  0.0), .init(0.0, 1.0)),
		Vertex(.init(-1.0,  1.0,  1.0), .init( 0.0,  1.0,  0.0), .init(0.0, 0.0)),
		Vertex(.init( 1.0,  1.0,  1.0), .init( 0.0,  1.0,  0.0), .init(1.0, 0.0)),
		Vertex(.init( 1.0,  1.0, -1.0), .init( 0.0,  1.0,  0.0), .init(1.0, 1.0)),
		// Bottom Face
		Vertex(.init(-1.0, -1.0, -1.0), .init( 0.0, -1.0,  0.0), .init(1.0, 1.0)),
		Vertex(.init( 1.0, -1.0, -1.0), .init( 0.0, -1.0,  0.0), .init(0.0, 1.0)),
		Vertex(.init( 1.0, -1.0,  1.0), .init( 0.0, -1.0,  0.0), .init(0.0, 0.0)),
		Vertex(.init(-1.0, -1.0,  1.0), .init( 0.0, -1.0,  0.0), .init(1.0, 0.0)),
		// Right face
		Vertex(.init( 1.0, -1.0, -1.0), .init( 1.0,  0.0,  0.0), .init(1.0, 0.0)),
		Vertex(.init( 1.0,  1.0, -1.0), .init( 1.0,  0.0,  0.0), .init(1.0, 1.0)),
		Vertex(.init( 1.0,  1.0,  1.0), .init( 1.0,  0.0,  0.0), .init(0.0, 1.0)),
		Vertex(.init( 1.0, -1.0,  1.0), .init( 1.0,  0.0,  0.0), .init(0.0, 0.0)),
		// Left Face
		Vertex(.init(-1.0, -1.0, -1.0), .init(-1.0,  0.0,  0.0), .init(0.0, 0.0)),
		Vertex(.init(-1.0, -1.0,  1.0), .init(-1.0,  0.0,  0.0), .init(1.0, 0.0)),
		Vertex(.init(-1.0,  1.0,  1.0), .init(-1.0,  0.0,  0.0), .init(1.0, 1.0)),
		Vertex(.init(-1.0,  1.0, -1.0), .init(-1.0,  0.0,  0.0), .init(0.0, 1.0)),
	]

	static let indices: [UInt16] =
	[
		 0,  1,  2,   2,  3,  0,  // Front
		 4,  5,  6,   6,  7,  4,  // Back
		 8,  9, 10,  10, 11,  8,  // Top
		12, 13, 14,  14, 15, 12,  // Bottom
		16, 17, 18,  18, 19, 16,  // Right
		20, 21, 22,  22, 23, 20   // Left
	]

	var psoUnlit: OpaquePointer? = nil
	var psoLight: OpaquePointer? = nil
	var vtxBuffer: OpaquePointer? = nil
	var idxBuffer: OpaquePointer? = nil
	var samplers = [OpaquePointer?](repeating: nil, count: 3)
	var texture: OpaquePointer? = nil
	var projection: matrix_float4x4 = .init(1.0)

	struct Light { let ambient: SIMD4<Float>, diffuse: SIMD4<Float>, position: SIMD4<Float> }

	var lighting = false
	var light = Light(
		ambient:  .init(0.5, 0.5, 0.5, 1.0),
		diffuse:  .init(1.0, 1.0, 1.0, 1.0),
		position: .init(0.0, 0.0, 2.0, 1.0))
	var filter = 0

	var rot   = SIMD2<Float>(repeating: 0.0)
	var speed = SIMD2<Float>(repeating: 0.0)
	var z: Float = -5.0

	mutating func `init`(ctx: inout NeHeContext) throws(NeHeError)
	{
		let (vertexShaderUnlit, fragmentShaderUnlit) = try ctx.loadShaders(name: "lesson6",
			vertexUniforms: 1, fragmentSamplers: 1)
		defer
		{
			SDL_ReleaseGPUShader(ctx.device, fragmentShaderUnlit)
			SDL_ReleaseGPUShader(ctx.device, vertexShaderUnlit)
		}
		let (vertexShaderLight, fragmentShaderLight) = try ctx.loadShaders(name: "lesson7",
			vertexUniforms: 2, fragmentSamplers: 1)
		defer
		{
			SDL_ReleaseGPUShader(ctx.device, fragmentShaderLight)
			SDL_ReleaseGPUShader(ctx.device, vertexShaderLight)
		}

		let vertexDescriptions: [SDL_GPUVertexBufferDescription] =
		[
			SDL_GPUVertexBufferDescription(
				slot: 0,
				pitch: UInt32(MemoryLayout<Vertex>.stride),
				input_rate: SDL_GPU_VERTEXINPUTRATE_VERTEX,
				instance_step_rate: 0),
		]
		let vertexAttributes: [SDL_GPUVertexAttribute] =
		[
			SDL_GPUVertexAttribute(
				location: 0,
				buffer_slot: 0,
				format: SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3,
				offset: UInt32(MemoryLayout<Vertex>.offset(of: \.position)!)),
			SDL_GPUVertexAttribute(
				location: 2,
				buffer_slot: 0,
				format: SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3,
				offset: UInt32(MemoryLayout<Vertex>.offset(of: \.normal)!)),
			SDL_GPUVertexAttribute(
				location: 1,
				buffer_slot: 0,
				format: SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2,
				offset: UInt32(MemoryLayout<Vertex>.offset(of: \.texcoord)!)),
		]

		var info = SDL_GPUGraphicsPipelineCreateInfo()
		info.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST
		info.vertex_input_state = SDL_GPUVertexInputState(
			vertex_buffer_descriptions: vertexDescriptions.withUnsafeBufferPointer(\.baseAddress!),
			num_vertex_buffers: UInt32(vertexDescriptions.count),
			vertex_attributes: vertexAttributes.withUnsafeBufferPointer(\.baseAddress!),
			num_vertex_attributes: UInt32(vertexAttributes.count))
		info.rasterizer_state.fill_mode  = SDL_GPU_FILLMODE_FILL
		info.rasterizer_state.cull_mode  = SDL_GPU_CULLMODE_NONE
		info.rasterizer_state.front_face = SDL_GPU_FRONTFACE_COUNTER_CLOCKWISE
		info.depth_stencil_state.compare_op         = SDL_GPU_COMPAREOP_LESS_OR_EQUAL
		info.depth_stencil_state.enable_depth_test  = true
		info.depth_stencil_state.enable_depth_write = true
		let colorTargets =
		[
			SDL_GPUColorTargetDescription(
				format: SDL_GetGPUSwapchainTextureFormat(ctx.device, ctx.window),
				blend_state: SDL_GPUColorTargetBlendState()),
		]
		info.target_info.color_target_descriptions = colorTargets.withUnsafeBufferPointer(\.baseAddress!)
		info.target_info.num_color_targets         = UInt32(colorTargets.count)
		info.target_info.depth_stencil_format      = SDL_GPU_TEXTUREFORMAT_D16_UNORM
		info.target_info.has_depth_stencil_target  = true

		// Create unlit pipeline
		info.vertex_shader   = vertexShaderUnlit
		info.fragment_shader = fragmentShaderUnlit
		guard let psoUnlit = SDL_CreateGPUGraphicsPipeline(ctx.device, &info) else
		{
			throw .sdlError("SDL_CreateGPUGraphicsPipeline", String(cString: SDL_GetError()))
		}
		self.psoUnlit = psoUnlit

		// Create lit pipeline
		info.vertex_shader   = vertexShaderLight
		info.fragment_shader = fragmentShaderLight
		guard let psoLight = SDL_CreateGPUGraphicsPipeline(ctx.device, &info) else
		{
			throw .sdlError("SDL_CreateGPUGraphicsPipeline", String(cString: SDL_GetError()))
		}
		self.psoLight = psoLight

		func createSampler(
			filter: SDL_GPUFilter,
			mipMode: SDL_GPUSamplerMipmapMode = SDL_GPU_SAMPLERMIPMAPMODE_NEAREST,
			maxLod: Float = 0.0) throws(NeHeError) -> OpaquePointer
		{
			var samplerInfo = SDL_GPUSamplerCreateInfo()
			samplerInfo.min_filter = filter
			samplerInfo.mag_filter = filter
			samplerInfo.mipmap_mode = mipMode
			samplerInfo.max_lod = maxLod
			guard let sampler = SDL_CreateGPUSampler(ctx.device, &samplerInfo) else
			{
				throw .sdlError("SDL_CreateGPUSampler", String(cString: SDL_GetError()))
			}
			return sampler
		}
		self.samplers[0] = try createSampler(filter: SDL_GPU_FILTER_NEAREST)
		self.samplers[1] = try createSampler(filter: SDL_GPU_FILTER_LINEAR)
		self.samplers[2] = try createSampler(filter: SDL_GPU_FILTER_LINEAR, maxLod: .greatestFiniteMagnitude)

		try ctx.copyPass { (pass) throws(NeHeError) in
			self.texture = try pass.createTextureFrom(bmpResource: "Crate", flip: true, genMipmaps: true)
			self.vtxBuffer = try pass.createBuffer(usage: SDL_GPU_BUFFERUSAGE_VERTEX, Self.vertices[...])
			self.idxBuffer = try pass.createBuffer(usage: SDL_GPU_BUFFERUSAGE_INDEX, Self.indices[...])
		}
	}

	func quit(ctx: NeHeContext)
	{
		SDL_ReleaseGPUBuffer(ctx.device, self.idxBuffer)
		SDL_ReleaseGPUBuffer(ctx.device, self.vtxBuffer)
		SDL_ReleaseGPUTexture(ctx.device, self.texture)
		self.samplers.reversed().forEach { SDL_ReleaseGPUSampler(ctx.device, $0) }
		SDL_ReleaseGPUGraphicsPipeline(ctx.device, self.psoLight)
		SDL_ReleaseGPUGraphicsPipeline(ctx.device, self.psoUnlit)
	}

	mutating func resize(size: Size<Int32>)
	{
		let aspect = Float(size.width) / Float(max(1, size.height))
		self.projection = .perspective(fovy: 45, aspect: aspect, near: 0.1, far: 100)
	}

	mutating func draw(ctx: inout NeHeContext, cmd: OpaquePointer,
		swapchain: OpaquePointer, swapchainSize: Size<UInt32>) throws(NeHeError)
	{
		var colorInfo = SDL_GPUColorTargetInfo()
		colorInfo.texture     = swapchain
		colorInfo.clear_color = SDL_FColor(r: 0.0, g: 0.0, b: 0.0, a: 0.5)
		colorInfo.load_op     = SDL_GPU_LOADOP_CLEAR
		colorInfo.store_op    = SDL_GPU_STOREOP_STORE

		var depthInfo = SDL_GPUDepthStencilTargetInfo()
		depthInfo.texture          = ctx.depthTexture
		depthInfo.clear_depth      = 1.0
		depthInfo.load_op          = SDL_GPU_LOADOP_CLEAR
		depthInfo.store_op         = SDL_GPU_STOREOP_DONT_CARE
		depthInfo.stencil_load_op  = SDL_GPU_LOADOP_DONT_CARE
		depthInfo.stencil_store_op = SDL_GPU_STOREOP_DONT_CARE
		depthInfo.cycle            = true

		// Begin pass & bind pipeline state
		let pass = SDL_BeginGPURenderPass(cmd, &colorInfo, 1, &depthInfo)
		SDL_BindGPUGraphicsPipeline(pass, self.lighting ? self.psoLight : self.psoUnlit)

		// Bind texture
		var textureBinding = SDL_GPUTextureSamplerBinding(texture: self.texture, sampler: self.samplers[self.filter])
		SDL_BindGPUFragmentSamplers(pass, 0, &textureBinding, 1)

		// Bind vertex & index buffers
		let vtxBindings = [ SDL_GPUBufferBinding(buffer: self.vtxBuffer, offset: 0) ]
		var idxBinding = SDL_GPUBufferBinding(buffer: self.idxBuffer, offset: 0)
		SDL_BindGPUVertexBuffers(pass, 0,
			vtxBindings.withUnsafeBufferPointer(\.baseAddress!), UInt32(vtxBindings.count))
		SDL_BindGPUIndexBuffer(pass, &idxBinding, SDL_GPU_INDEXELEMENTSIZE_16BIT)

		// Setup the cube's model matrix
		var model: simd_float4x4 = .translation(.init(0.0, 0.0, self.z))
		model.rotate(angle: rot.x, axis: .init(1.0, 0.0, 0.0))
		model.rotate(angle: rot.y, axis: .init(0.0, 1.0, 0.0))

		// Push shader uniforms
		if self.lighting
		{
			struct Uniforms { var model: simd_float4x4, projection: simd_float4x4 }
			var u = Uniforms(
				model: model,
				projection: self.projection)
			SDL_PushGPUVertexUniformData(cmd, 0, &u, UInt32(MemoryLayout<Uniforms>.size))
			SDL_PushGPUVertexUniformData(cmd, 1, &self.light, UInt32(MemoryLayout<Light>.size))
		}
		else
		{
			var modelViewProj = self.projection * model
			SDL_PushGPUVertexUniformData(cmd, 0, &modelViewProj, UInt32(MemoryLayout<simd_float4x4>.size))
		}

		// Draw textured cube
		SDL_DrawGPUIndexedPrimitives(pass, UInt32(Self.indices.count), 1, 0, 0, 0)

		SDL_EndGPURenderPass(pass)

		let keys = SDL_GetKeyboardState(nil)!

		if keys[Int(SDL_SCANCODE_PAGEUP.rawValue)]   { self.z -= 0.02 }
		if keys[Int(SDL_SCANCODE_PAGEDOWN.rawValue)] { self.z += 0.02 }
		if keys[Int(SDL_SCANCODE_UP.rawValue)]   { speed.x -= 0.01 }
		if keys[Int(SDL_SCANCODE_DOWN.rawValue)] { speed.x += 0.01 }
		if keys[Int(SDL_SCANCODE_RIGHT.rawValue)] { speed.y += 0.1 }
		if keys[Int(SDL_SCANCODE_LEFT.rawValue)]  { speed.y -= 0.1 }

		self.rot += self.speed
	}

	mutating func key(ctx: inout NeHeContext, key: SDL_Keycode, down: Bool, repeat: Bool)
	{
		guard down && !`repeat` else { return }
		switch key
		{
		case SDLK_L:
			self.lighting = !self.lighting
		case SDLK_F:
			self.filter = (self.filter + 1) % self.samplers.count
		default:
			break
		}
	}
}

@main struct Program: AppRunner
{
	typealias Delegate = Lesson7
	static let config = AppConfig(
		title: "NeHe's Textures, Lighting & Keyboard Tutorial",
		width: 640,
		height: 480,
		createDepthBuffer: SDL_GPU_TEXTUREFORMAT_D16_UNORM,
		bundle: Bundle.module)
}
