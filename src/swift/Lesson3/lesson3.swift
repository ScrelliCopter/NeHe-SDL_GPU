/*
 * SPDX-FileCopyrightText: (C) 2025 a dinosaur
 * SPDX-License-Identifier: Zlib
 */

import Foundation
import SDLSwift
import NeHe
import simd

struct Lesson3: AppDelegate
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
		// Triangle
		Vertex(.init( 0.0,  1.0, 0.0), .init(1.0, 0.0, 0.0, 1.0)),  // Top (red)
		Vertex(.init(-1.0, -1.0, 0.0), .init(0.0, 1.0, 0.0, 1.0)),  // Bottom left (green)
		Vertex(.init( 1.0, -1.0, 0.0), .init(0.0, 0.0, 1.0, 1.0)),  // Bottom right (blue)
		// Quad
		Vertex(.init(-1.0,  1.0, 0.0), .init(0.5, 0.5, 1.0, 1.0)),  // Top left
		Vertex(.init( 1.0,  1.0, 0.0), .init(0.5, 0.5, 1.0, 1.0)),  // Top right
		Vertex(.init( 1.0, -1.0, 0.0), .init(0.5, 0.5, 1.0, 1.0)),  // Bottom right
		Vertex(.init(-1.0, -1.0, 0.0), .init(0.5, 0.5, 1.0, 1.0)),  // Bottom left
	]

	static let indices: [Int16] =
	[
		// Triangle
		0, 1, 2,
		// Quad
		3, 4, 5, 5, 6, 3,
	]

	var pso: OpaquePointer? = nil
	var vtxBuffer: OpaquePointer? = nil
	var idxBuffer: OpaquePointer? = nil
	var projection: matrix_float4x4 = .init(1.0)

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
		let colourTargets: [SDL_GPUColorTargetDescription] =
		[
			SDL_GPUColorTargetDescription(
				format: SDL_GetGPUSwapchainTextureFormat(ctx.device, ctx.window),
				blend_state: SDL_GPUColorTargetBlendState())
		]
		var rasterizerDesc = SDL_GPURasterizerState()
		rasterizerDesc.fill_mode  = SDL_GPU_FILLMODE_FILL
		rasterizerDesc.cull_mode  = SDL_GPU_CULLMODE_NONE
		rasterizerDesc.front_face = SDL_GPU_FRONTFACE_COUNTER_CLOCKWISE
		var targetInfo = SDL_GPUGraphicsPipelineTargetInfo()
		targetInfo.color_target_descriptions = colourTargets.withUnsafeBufferPointer(\.baseAddress!)
		targetInfo.num_color_targets         = UInt32(colourTargets.count)

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
			depth_stencil_state: SDL_GPUDepthStencilState(),
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

	func draw(ctx: inout NeHeContext, cmd: OpaquePointer,
		swapchain: OpaquePointer, swapchainSize: Size<UInt32>) throws(NeHeError)
	{
		var colorInfo = SDL_GPUColorTargetInfo()
		colorInfo.texture     = swapchain
		colorInfo.clear_color = SDL_FColor(r: 0.0, g: 0.0, b: 0.0, a: 0.5)
		colorInfo.load_op     = SDL_GPU_LOADOP_CLEAR
		colorInfo.store_op    = SDL_GPU_STOREOP_STORE

		// Begin pass & bind pipeline state
		let pass = SDL_BeginGPURenderPass(cmd, &colorInfo, 1, nil);
		SDL_BindGPUGraphicsPipeline(pass, self.pso)

		// Bind vertex & index buffers
		let vtxBindings = [ SDL_GPUBufferBinding(buffer: self.vtxBuffer, offset: 0) ]
		var idxBinding = SDL_GPUBufferBinding(buffer: self.idxBuffer, offset: 0)
		SDL_BindGPUVertexBuffers(pass, 0,
			vtxBindings.withUnsafeBufferPointer(\.baseAddress!), UInt32(vtxBindings.count))
		SDL_BindGPUIndexBuffer(pass, &idxBinding, SDL_GPU_INDEXELEMENTSIZE_16BIT)

		// Draw triangle 1.5 units to the left and 6 units into the camera
		var model: simd_float4x4 = .translation(.init(-1.5, 0.0, -6.0))
		var viewProj = self.projection * model
		SDL_PushGPUVertexUniformData(cmd, 0, &viewProj, UInt32(MemoryLayout<simd_float4x4>.size))
		SDL_DrawGPUIndexedPrimitives(pass, 3, 1, 0, 0, 0)

		// Move to the right by 3 units and draw quad
		model.translate(.init(3.0, 0.0, 0.0))
		viewProj = self.projection * model
		SDL_PushGPUVertexUniformData(cmd, 0, &viewProj, UInt32(MemoryLayout<simd_float4x4>.size))
		SDL_DrawGPUIndexedPrimitives(pass, 6, 1, 3, 0, 0)

		SDL_EndGPURenderPass(pass);
	}
}

@main struct Program: AppRunner
{
	typealias Delegate = Lesson3
	static let config = AppConfig(
		title: "NeHe's Color Tutorial",
		width: 640,
		height: 480,
		bundle: Bundle.module)
}
