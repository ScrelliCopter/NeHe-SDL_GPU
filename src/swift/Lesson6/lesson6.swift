/*
 * SPDX-FileCopyrightText: (C) 2025 a dinosaur
 * SPDX-License-Identifier: Zlib
 */

import Foundation
import SDLSwift
import NeHe
import simd

struct Lesson6: AppDelegate
{
	struct Vertex
	{
		let position: SIMD3<Float>, texcoord: SIMD2<Float>

		init(_ position: SIMD3<Float>, _ texcoord: SIMD2<Float>)
		{
			self.position = position
			self.texcoord = texcoord
		}
	}

	static let vertices =
	[
		// Front Face
		Vertex(.init(-1.0, -1.0,  1.0), .init(0.0, 0.0)),
		Vertex(.init( 1.0, -1.0,  1.0), .init(1.0, 0.0)),
		Vertex(.init( 1.0,  1.0,  1.0), .init(1.0, 1.0)),
		Vertex(.init(-1.0,  1.0,  1.0), .init(0.0, 1.0)),
		// Back Face
		Vertex(.init(-1.0, -1.0, -1.0), .init(1.0, 0.0)),
		Vertex(.init(-1.0,  1.0, -1.0), .init(1.0, 1.0)),
		Vertex(.init( 1.0,  1.0, -1.0), .init(0.0, 1.0)),
		Vertex(.init( 1.0, -1.0, -1.0), .init(0.0, 0.0)),
		// Top Face
		Vertex(.init(-1.0,  1.0, -1.0), .init(0.0, 1.0)),
		Vertex(.init(-1.0,  1.0,  1.0), .init(0.0, 0.0)),
		Vertex(.init( 1.0,  1.0,  1.0), .init(1.0, 0.0)),
		Vertex(.init( 1.0,  1.0, -1.0), .init(1.0, 1.0)),
		// Bottom Face
		Vertex(.init(-1.0, -1.0, -1.0), .init(1.0, 1.0)),
		Vertex(.init( 1.0, -1.0, -1.0), .init(0.0, 1.0)),
		Vertex(.init( 1.0, -1.0,  1.0), .init(0.0, 0.0)),
		Vertex(.init(-1.0, -1.0,  1.0), .init(1.0, 0.0)),
		// Right face
		Vertex(.init( 1.0, -1.0, -1.0), .init(1.0, 0.0)),
		Vertex(.init( 1.0,  1.0, -1.0), .init(1.0, 1.0)),
		Vertex(.init( 1.0,  1.0,  1.0), .init(0.0, 1.0)),
		Vertex(.init( 1.0, -1.0,  1.0), .init(0.0, 0.0)),
		// Left Face
		Vertex(.init(-1.0, -1.0, -1.0), .init(0.0, 0.0)),
		Vertex(.init(-1.0, -1.0,  1.0), .init(1.0, 0.0)),
		Vertex(.init(-1.0,  1.0,  1.0), .init(1.0, 1.0)),
		Vertex(.init(-1.0,  1.0, -1.0), .init(0.0, 1.0)),
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

	var pso: OpaquePointer? = nil
	var vtxBuffer: OpaquePointer? = nil
	var idxBuffer: OpaquePointer? = nil
	var sampler: OpaquePointer? = nil
	var texture: OpaquePointer? = nil
	var projection: matrix_float4x4 = .init(1.0)

	var rot: SIMD3<Float> = .init(repeating: 0.0)

	mutating func `init`(ctx: inout NeHeContext) throws(NeHeError)
	{
		let (vertexShader, fragmentShader) = try ctx.loadShaders(name: "lesson6",
			vertexUniforms: 1, fragmentSamplers: 1)
		defer
		{
			SDL_ReleaseGPUShader(ctx.device, fragmentShader)
			SDL_ReleaseGPUShader(ctx.device, vertexShader)
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
				location: 1,
				buffer_slot: 0,
				format: SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2,
				offset: UInt32(MemoryLayout<Vertex>.offset(of: \.texcoord)!)),
		]
		let colorTargets: [SDL_GPUColorTargetDescription] =
		[
			SDL_GPUColorTargetDescription(
				format: SDL_GetGPUSwapchainTextureFormat(ctx.device, ctx.window),
				blend_state: SDL_GPUColorTargetBlendState())
		]
		var rasterizerDesc = SDL_GPURasterizerState()
		rasterizerDesc.fill_mode  = SDL_GPU_FILLMODE_FILL
		rasterizerDesc.cull_mode  = SDL_GPU_CULLMODE_NONE
		rasterizerDesc.front_face = SDL_GPU_FRONTFACE_COUNTER_CLOCKWISE
		var depthStencilState = SDL_GPUDepthStencilState()
		depthStencilState.compare_op         = SDL_GPU_COMPAREOP_LESS_OR_EQUAL
		depthStencilState.enable_depth_test  = true
		depthStencilState.enable_depth_write = true
		var targetInfo = SDL_GPUGraphicsPipelineTargetInfo()
		targetInfo.color_target_descriptions = colorTargets.withUnsafeBufferPointer(\.baseAddress!)
		targetInfo.num_color_targets         = UInt32(colorTargets.count)
		targetInfo.depth_stencil_format      = SDL_GPU_TEXTUREFORMAT_D16_UNORM
		targetInfo.has_depth_stencil_target  = true

		var info = SDL_GPUGraphicsPipelineCreateInfo(
			vertex_shader: vertexShader,
			fragment_shader: fragmentShader,
			vertex_input_state: SDL_GPUVertexInputState(
				vertex_buffer_descriptions: vertexDescriptions.withUnsafeBufferPointer(\.baseAddress!),
				num_vertex_buffers: UInt32(vertexDescriptions.count),
				vertex_attributes: vertexAttributes.withUnsafeBufferPointer(\.baseAddress!),
				num_vertex_attributes: UInt32(vertexAttributes.count)),
			primitive_type: SDL_GPU_PRIMITIVETYPE_TRIANGLELIST,
			rasterizer_state: rasterizerDesc,
			multisample_state: SDL_GPUMultisampleState(),
			depth_stencil_state: depthStencilState,
			target_info: targetInfo,
			props: 0)
		guard let pso = SDL_CreateGPUGraphicsPipeline(ctx.device, &info) else
		{
			throw .sdlError("SDL_CreateGPUGraphicsPipeline", String(cString: SDL_GetError()))
		}
		self.pso = pso

		var samplerInfo = SDL_GPUSamplerCreateInfo()
		samplerInfo.min_filter = SDL_GPU_FILTER_LINEAR
		samplerInfo.mag_filter = SDL_GPU_FILTER_LINEAR
		guard let sampler = SDL_CreateGPUSampler(ctx.device, &samplerInfo) else
		{
			throw .sdlError("SDL_CreateGPUSampler", String(cString: SDL_GetError()))
		}
		self.sampler = sampler

		try ctx.copyPass { (pass) throws(NeHeError) in
			self.texture = try pass.createTextureFrom(bmpResource: "NeHe", flip: true)
			self.vtxBuffer = try pass.createBuffer(usage: SDL_GPU_BUFFERUSAGE_VERTEX, Self.vertices[...])
			self.idxBuffer = try pass.createBuffer(usage: SDL_GPU_BUFFERUSAGE_INDEX, Self.indices[...])
		}
	}

	func quit(ctx: NeHeContext)
	{
		SDL_ReleaseGPUBuffer(ctx.device, self.idxBuffer)
		SDL_ReleaseGPUBuffer(ctx.device, self.vtxBuffer)
		SDL_ReleaseGPUTexture(ctx.device, self.texture)
		SDL_ReleaseGPUSampler(ctx.device, self.sampler)
		SDL_ReleaseGPUGraphicsPipeline(ctx.device, self.pso)
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
		SDL_BindGPUGraphicsPipeline(pass, self.pso)

		// Bind texture
		var textureBinding = SDL_GPUTextureSamplerBinding(texture: self.texture, sampler: self.sampler)
		SDL_BindGPUFragmentSamplers(pass, 0, &textureBinding, 1)

		// Bind vertex & index buffers
		let vtxBindings = [ SDL_GPUBufferBinding(buffer: self.vtxBuffer, offset: 0) ]
		var idxBinding = SDL_GPUBufferBinding(buffer: self.idxBuffer, offset: 0)
		SDL_BindGPUVertexBuffers(pass, 0,
			vtxBindings.withUnsafeBufferPointer(\.baseAddress!), UInt32(vtxBindings.count))
		SDL_BindGPUIndexBuffer(pass, &idxBinding, SDL_GPU_INDEXELEMENTSIZE_16BIT)

		// Move cube 5 units into the camera and apply some rotations
		var model: simd_float4x4 = .translation(.init(0.0, 0.0, -5.0))
		model.rotate(angle: rot.x, axis: .init(1.0, 0.0, 0.0))
		model.rotate(angle: rot.y, axis: .init(0.0, 1.0, 0.0))
		model.rotate(angle: rot.z, axis: .init(0.0, 0.0, 1.0))

		// Push shader uniforms
		var modelViewProj = self.projection * model
		SDL_PushGPUVertexUniformData(cmd, 0, &modelViewProj, UInt32(MemoryLayout<matrix_float4x4>.size))

		// Draw textured cube
		SDL_DrawGPUIndexedPrimitives(pass, UInt32(Self.indices.count), 1, 0, 0, 0)

		SDL_EndGPURenderPass(pass)

		self.rot += .init(0.3, 0.2, 0.4)
	}
}

@main struct Program: AppRunner
{
	typealias Delegate = Lesson6
	static let config = AppConfig(
		title: "NeHe's Texture Mapping Tutorial",
		width: 640,
		height: 480,
		createDepthBuffer: SDL_GPU_TEXTUREFORMAT_D16_UNORM,
		bundle: Bundle.module)
}
