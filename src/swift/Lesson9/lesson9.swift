/*
 * SPDX-FileCopyrightText: (C) 2025 a dinosaur
 * SPDX-License-Identifier: Zlib
 */

import Foundation
import SDLSwift
import NeHe
import simd

struct Lesson9: AppDelegate
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

	struct Instance
	{
		let model: matrix_float4x4
		let color: SIMD4<Float>
	}

	struct Star
	{
		var angle: Float
		var distance: Float
		var color: SIMD3<UInt8>
	}

	static let vertices =
	[
		Vertex(.init(-1.0, -1.0, 0.0), .init(0.0, 0.0)),
		Vertex(.init( 1.0, -1.0, 0.0), .init(1.0, 0.0)),
		Vertex(.init( 1.0,  1.0, 0.0), .init(1.0, 1.0)),
		Vertex(.init(-1.0,  1.0, 0.0), .init(0.0, 1.0)),
	]

	static let indices: [UInt16] =
	[
		0,  1,  2,
		2,  3,  0,
	]

	var pso: OpaquePointer? = nil
	var vtxBuffer: OpaquePointer? = nil
	var idxBuffer: OpaquePointer? = nil
	var instanceBuffer: OpaquePointer? = nil
	var instanceXferBuffer: OpaquePointer? = nil
	var sampler: OpaquePointer? = nil
	var texture: OpaquePointer? = nil

	var projection: matrix_float4x4 = .init(1.0)

	var twinkle = false
	var stars: [Star] = []

	var zoom: Float = -15.0
	var tilt: Float = 90.0
	var spin: Float = 0.0

	var random = NeHeRandom()

	mutating func `init`(ctx: inout NeHeContext) throws(NeHeError)
	{
		let (vertexShader, fragmentShader) = try ctx.loadShaders(name: "lesson9",
			vertexUniforms: 1, vertexStorage: 1, fragmentSamplers: 1)
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

		var rasterizerDesc = SDL_GPURasterizerState()
		rasterizerDesc.fill_mode  = SDL_GPU_FILLMODE_FILL
		rasterizerDesc.cull_mode  = SDL_GPU_CULLMODE_NONE
		rasterizerDesc.front_face = SDL_GPU_FRONTFACE_COUNTER_CLOCKWISE

		var colorTargets = [ SDL_GPUColorTargetDescription() ]
		colorTargets[0].format = SDL_GetGPUSwapchainTextureFormat(ctx.device, ctx.window)
		colorTargets[0].blend_state.enable_blend = true
		colorTargets[0].blend_state.color_blend_op = SDL_GPU_BLENDOP_ADD
		colorTargets[0].blend_state.alpha_blend_op = SDL_GPU_BLENDOP_ADD
		colorTargets[0].blend_state.src_color_blendfactor = SDL_GPU_BLENDFACTOR_SRC_ALPHA
		colorTargets[0].blend_state.dst_color_blendfactor = SDL_GPU_BLENDFACTOR_ONE
		colorTargets[0].blend_state.src_alpha_blendfactor = SDL_GPU_BLENDFACTOR_SRC_ALPHA
		colorTargets[0].blend_state.dst_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE

		var targetInfo = SDL_GPUGraphicsPipelineTargetInfo()
		targetInfo.color_target_descriptions = colorTargets.withUnsafeBufferPointer(\.baseAddress!)
		targetInfo.num_color_targets = UInt32(colorTargets.count)

		var psoInfo = SDL_GPUGraphicsPipelineCreateInfo(
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
		guard let pso = SDL_CreateGPUGraphicsPipeline(ctx.device, &psoInfo) else
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
			self.texture = try pass.createTextureFrom(bmpResource: "Star", flip: true, genMipmaps: false)
			self.vtxBuffer = try pass.createBuffer(usage: SDL_GPU_BUFFERUSAGE_VERTEX, Self.vertices[...])
			self.idxBuffer = try pass.createBuffer(usage: SDL_GPU_BUFFERUSAGE_INDEX, Self.indices[...])
		}

		let numStars = 50

		// Create GPU side buffer for star instances
		let instanceBufferSize = UInt32(MemoryLayout<Instance>.stride * 2 * numStars)
		var instanceInfo = SDL_GPUBufferCreateInfo(
			usage: SDL_GPU_BUFFERUSAGE_GRAPHICS_STORAGE_READ,
			size: instanceBufferSize,
			props: 0)
		guard let instanceBuffer = SDL_CreateGPUBuffer(ctx.device, &instanceInfo) else
		{
			throw .sdlError("SDL_CreateGPUBuffer", String(cString: SDL_GetError()))
		}
		self.instanceBuffer = instanceBuffer

		// Create CPU side buffer for star instances (to upload to GPU)
		var instanceXferInfo = SDL_GPUTransferBufferCreateInfo(
			usage: SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
			size: instanceBufferSize,
			props: 0)
		guard let instanceXferBuffer = SDL_CreateGPUTransferBuffer(ctx.device, &instanceXferInfo) else
		{
			throw .sdlError("SDL_CreateGPUTransferBuffer", String(cString: SDL_GetError()))
		}
		self.instanceXferBuffer = instanceXferBuffer

		// Initialise stars
		self.stars = (0..<numStars).map
		{ i in .init(
				angle: 0.0,
				distance: 5.0 * (Float(i) / Float(numStars)),
				color: .init(
					UInt8(truncatingIfNeeded: self.random.next() % 256),
					UInt8(truncatingIfNeeded: self.random.next() % 256),
					UInt8(truncatingIfNeeded: self.random.next() % 256)))
		}
	}

	func quit(ctx: NeHeContext)
	{
		SDL_ReleaseGPUTransferBuffer(ctx.device, self.instanceXferBuffer)
		SDL_ReleaseGPUBuffer(ctx.device, self.instanceBuffer)
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
		// Animate stars
		let numStars = self.stars.count
		let numInstances = self.twinkle ? 2 * numStars : numStars
		let map = SDL_MapGPUTransferBuffer(ctx.device, self.instanceXferBuffer, true)!
		map.withMemoryRebound(to: Instance.self, capacity: numInstances)
		{ instances in
			var instanceIDX = 0
			for (starIDX, star) in self.stars.enumerated()
			{
				var model = matrix_float4x4.translation(.init(0.0, 0.0, zoom))
				model.rotate(angle:        tilt, axis: .init(1.0, 0.0, 0.0))
				model.rotate(angle:  star.angle, axis: .init(0.0, 1.0, 0.0))
				model.translate(.init(star.distance, 0.0, 0.0))
				model.rotate(angle: -star.angle, axis: .init(0.0, 1.0, 0.0))
				model.rotate(angle:       -tilt, axis: .init(1.0, 0.0, 0.0))

				if self.twinkle
				{
					let twinkleColor = stars[numStars - starIDX - 1].color
					instances[instanceIDX] = Instance(model: model,
						color: SIMD4(SIMD3<Float>(twinkleColor) / 255.0, 1.0))
					instanceIDX += 1
				}

				model.rotate(angle: self.spin, axis: .init(0.0, 0.0, 1.0))
				instances[instanceIDX] = Instance(model: model, color: SIMD4(SIMD3(star.color) / 255.0, 1.0))
				instanceIDX += 1

				self.spin += 0.01

				// Update star
				var newStar = star
				newStar.angle += Float(starIDX) / Float(numStars)
				newStar.distance -= 0.01
				if newStar.distance < 0.0
				{
					newStar.distance += 5.0
					newStar.color = SIMD3(newStar.color.indices.map { _ in
						UInt8(truncatingIfNeeded: self.random.next() % 256)
					})
				}
				self.stars[starIDX] = newStar
			}
		}
		SDL_UnmapGPUTransferBuffer(ctx.device, self.instanceXferBuffer)

		// Upload instances buffer to the GPU
		let copyPass = SDL_BeginGPUCopyPass(cmd)
		var source = SDL_GPUTransferBufferLocation(
			transfer_buffer: self.instanceXferBuffer,
			offset: 0)
		var destination = SDL_GPUBufferRegion(
			buffer: self.instanceBuffer,
			offset: 0,
			size: UInt32(MemoryLayout<Instance>.stride * numInstances))
		SDL_UploadToGPUBuffer(copyPass, &source, &destination, true)
		SDL_EndGPUCopyPass(copyPass)

		// Begin pass & bind pipeline state
		var colorInfo = SDL_GPUColorTargetInfo()
		colorInfo.texture     = swapchain
		colorInfo.clear_color = SDL_FColor(r: 0.0, g: 0.0, b: 0.0, a: 0.5)
		colorInfo.load_op     = SDL_GPU_LOADOP_CLEAR
		colorInfo.store_op    = SDL_GPU_STOREOP_STORE
		let renderPass = SDL_BeginGPURenderPass(cmd, &colorInfo, 1, nil)
		SDL_BindGPUGraphicsPipeline(renderPass, self.pso)

		// Bind particle texture
		var textureBinding = SDL_GPUTextureSamplerBinding(texture: self.texture, sampler: self.sampler)
		SDL_BindGPUFragmentSamplers(renderPass, 0, &textureBinding, 1)

		// Bind vertex & index buffers
		let vtxBindings = [ SDL_GPUBufferBinding(buffer: self.vtxBuffer, offset: 0) ]
		var idxBinding = SDL_GPUBufferBinding(buffer: self.idxBuffer, offset: 0)
		SDL_BindGPUVertexBuffers(renderPass, 0,
			vtxBindings.withUnsafeBufferPointer(\.baseAddress!), UInt32(vtxBindings.count))
		SDL_BindGPUIndexBuffer(renderPass, &idxBinding, SDL_GPU_INDEXELEMENTSIZE_16BIT)

		// Bind instance storage buffer
		SDL_BindGPUVertexStorageBuffers(renderPass, 0, &self.instanceBuffer, 1)

		// Push shader uniforms
		SDL_PushGPUVertexUniformData(cmd, 0, &self.projection, UInt32(MemoryLayout<matrix_float4x4>.size))

		// Draw star instances
		SDL_DrawGPUIndexedPrimitives(renderPass, UInt32(Self.indices.count), UInt32(numInstances), 0, 0, 0)

		SDL_EndGPURenderPass(renderPass)

		let keys = SDL_GetKeyboardState(nil)!

		if keys[Int(SDL_SCANCODE_UP.rawValue)]       { tilt -= 0.5 }
		if keys[Int(SDL_SCANCODE_DOWN.rawValue)]     { tilt += 0.5 }
		if keys[Int(SDL_SCANCODE_PAGEUP.rawValue)]   { zoom -= 0.2 }
		if keys[Int(SDL_SCANCODE_PAGEDOWN.rawValue)] { zoom += 0.2 }
	}

	mutating func key(ctx: inout NeHeContext, key: SDL_Keycode, down: Bool, repeat: Bool)
	{
		guard down && !`repeat` else { return }
		switch key
		{
		case SDLK_T:
			self.twinkle = !self.twinkle
		default:
			break
		}
	}
}

@main struct Program: AppRunner
{
	typealias Delegate = Lesson9
	static let config = AppConfig(
		title: "NeHe's Animated Blended Textures Tutorial",
		width: 640,
		height: 480,
		bundle: Bundle.module)
}
