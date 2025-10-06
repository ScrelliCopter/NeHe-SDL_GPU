/*
 * SPDX-FileCopyrightText: (C) 2025 a dinosaur
 * SPDX-License-Identifier: Zlib
 */

import Foundation
import SDLSwift
import NeHe
import simd

struct Lesson5: AppDelegate
{
	struct Vertex
	{
		let position: SIMD3<Float>, color: SIMD4<Float>

		init(_ position: SIMD3<Float>, _ color: SIMD4<Float>)
		{
			self.position = position
			self.color = color
		}
	}

	static let vertices =
	[
		// Pyramid
		Vertex(.init( 0.0,  1.0,  0.0), .init(1.0, 0.0, 0.0, 1.0)),  // Top of pyramid (Red)
		Vertex(.init(-1.0, -1.0,  1.0), .init(0.0, 1.0, 0.0, 1.0)),  // Front-left of pyramid (Green)
		Vertex(.init( 1.0, -1.0,  1.0), .init(0.0, 0.0, 1.0, 1.0)),  // Front-right of pyramid (Blue)
		Vertex(.init( 1.0, -1.0, -1.0), .init(0.0, 1.0, 0.0, 1.0)),  // Back-right of pyramid (Green)
		Vertex(.init(-1.0, -1.0, -1.0), .init(0.0, 0.0, 1.0, 1.0)),  // Back-left of pyramid (Blue)
		// Cube
		Vertex(.init( 1.0,  1.0, -1.0), .init(0.0, 1.0, 0.0, 1.0)),  // Top-right of top face (Green)
		Vertex(.init(-1.0,  1.0, -1.0), .init(0.0, 1.0, 0.0, 1.0)),  // Top-left of top face (Green)
		Vertex(.init(-1.0,  1.0,  1.0), .init(0.0, 1.0, 0.0, 1.0)),  // Bottom-left of top face (Green)
		Vertex(.init( 1.0,  1.0,  1.0), .init(0.0, 1.0, 0.0, 1.0)),  // Bottom-right of top face (Green)
		Vertex(.init( 1.0, -1.0,  1.0), .init(1.0, 0.5, 0.0, 1.0)),  // Top-right of bottom face (Orange)
		Vertex(.init(-1.0, -1.0,  1.0), .init(1.0, 0.5, 0.0, 1.0)),  // Top-left of bottom face (Orange)
		Vertex(.init(-1.0, -1.0, -1.0), .init(1.0, 0.5, 0.0, 1.0)),  // Bottom-left of bottom face (Orange)
		Vertex(.init( 1.0, -1.0, -1.0), .init(1.0, 0.5, 0.0, 1.0)),  // Bottom-right of bottom face (Orange)
		Vertex(.init( 1.0,  1.0,  1.0), .init(1.0, 0.0, 0.0, 1.0)),  // Top-right of front face (Red)
		Vertex(.init(-1.0,  1.0,  1.0), .init(1.0, 0.0, 0.0, 1.0)),  // Top-left of front face (Red)
		Vertex(.init(-1.0, -1.0,  1.0), .init(1.0, 0.0, 0.0, 1.0)),  // Bottom-left of front face (Red)
		Vertex(.init( 1.0, -1.0,  1.0), .init(1.0, 0.0, 0.0, 1.0)),  // Bottom-right of front face (Red)
		Vertex(.init( 1.0, -1.0, -1.0), .init(1.0, 1.0, 0.0, 1.0)),  // Top-right of back face (Yellow)
		Vertex(.init(-1.0, -1.0, -1.0), .init(1.0, 1.0, 0.0, 1.0)),  // Top-left of back face (Yellow)
		Vertex(.init(-1.0,  1.0, -1.0), .init(1.0, 1.0, 0.0, 1.0)),  // Bottom-left of back face (Yellow)
		Vertex(.init( 1.0,  1.0, -1.0), .init(1.0, 1.0, 0.0, 1.0)),  // Bottom-right of back face (Yellow)
		Vertex(.init(-1.0,  1.0,  1.0), .init(0.0, 0.0, 1.0, 1.0)),  // Top-right of left face (Blue)
		Vertex(.init(-1.0,  1.0, -1.0), .init(0.0, 0.0, 1.0, 1.0)),  // Top-left of left face (Blue)
		Vertex(.init(-1.0, -1.0, -1.0), .init(0.0, 0.0, 1.0, 1.0)),  // Bottom-left of left face (Blue)
		Vertex(.init(-1.0, -1.0,  1.0), .init(0.0, 0.0, 1.0, 1.0)),  // Bottom-right of left face (Blue)
		Vertex(.init( 1.0,  1.0, -1.0), .init(1.0, 0.0, 1.0, 1.0)),  // Top-right of right face (Violet)
		Vertex(.init( 1.0,  1.0,  1.0), .init(1.0, 0.0, 1.0, 1.0)),  // Top-left of right face (Violet)
		Vertex(.init( 1.0, -1.0,  1.0), .init(1.0, 0.0, 1.0, 1.0)),  // Bottom-left of right face (Violet)
		Vertex(.init( 1.0, -1.0, -1.0), .init(1.0, 0.0, 1.0, 1.0)),  // Bottom-right of right face (Violet)
	]
	
	static let indices: [UInt16] =
	[
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
	]

	var pso: OpaquePointer? = nil
	var vtxBuffer: OpaquePointer? = nil
	var idxBuffer: OpaquePointer? = nil
	var projection: simd_float4x4 = .init(1.0)

	var rotTri: Float = 0.0, rotQuad: Float = 0.0

	mutating func `init`(ctx: inout NeHeContext) throws(NeHeError)
	{
		let (vertexShader, fragmentShader) = try ctx.loadShaders(name: "lesson3", vertexUniforms: 1)
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
				format: SDL_GPU_VERTEXELEMENTFORMAT_FLOAT4,
				offset: UInt32(MemoryLayout<Vertex>.offset(of: \.color)!)),
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
		targetInfo.depth_stencil_format      = ctx.depthTextureFormat
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

		try ctx.copyPass { (pass) throws(NeHeError) in
			self.vtxBuffer = try pass.createBuffer(usage: SDL_GPU_BUFFERUSAGE_VERTEX, Self.vertices[...])
			self.idxBuffer = try pass.createBuffer(usage: SDL_GPU_BUFFERUSAGE_INDEX, Self.indices[...])
		}
	}

	func quit(ctx: NeHeContext)
	{
		SDL_ReleaseGPUBuffer(ctx.device, self.idxBuffer)
		SDL_ReleaseGPUBuffer(ctx.device, self.vtxBuffer)
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

		// Bind vertex & index buffers
		let vtxBindings = [ SDL_GPUBufferBinding(buffer: self.vtxBuffer, offset: 0) ]
		var idxBinding = SDL_GPUBufferBinding(buffer: self.idxBuffer, offset: 0)
		SDL_BindGPUVertexBuffers(pass, 0,
			vtxBindings.withUnsafeBufferPointer(\.baseAddress!), UInt32(vtxBindings.count))
		SDL_BindGPUIndexBuffer(pass, &idxBinding, SDL_GPU_INDEXELEMENTSIZE_16BIT)

		// Draw pyramid 1.5 units to the left and 6 units into the camera
		var model: simd_float4x4 = .translation(.init(-1.5, 0.0, -6.0))
		model.rotate(angle: self.rotTri, axis: .init(0, 1, 0))
		var viewProj = self.projection * model
		SDL_PushGPUVertexUniformData(cmd, 0, &viewProj, UInt32(MemoryLayout<simd_float4x4>.size))
		SDL_DrawGPUIndexedPrimitives(pass, 12, 1, 0, 0, 0)

		// Draw cube 1.5 units to the right and 7 units into the camera
		model = .translation(.init(1.5, 0.0, -7.0))
		model.rotate(angle: self.rotQuad, axis: .init(1, 1, 1))
		viewProj = self.projection * model
		SDL_PushGPUVertexUniformData(cmd, 0, &viewProj, UInt32(MemoryLayout<simd_float4x4>.size))
		SDL_DrawGPUIndexedPrimitives(pass, 36, 1, 12, 0, 0)

		SDL_EndGPURenderPass(pass)

		self.rotTri += 0.2
		self.rotQuad -= 0.15
	}
}

@main struct Program: AppRunner
{
	typealias Delegate = Lesson5
	static let config = AppConfig(
		title: "NeHe's Solid Object Tutorial",
		width: 640,
		height: 480,
		createDepthBuffer: SDL_GPU_TEXTUREFORMAT_D16_UNORM,
		bundle: Bundle.module)
}
