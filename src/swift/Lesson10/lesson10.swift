/*
 * SPDX-FileCopyrightText: (C) 2025 a dinosaur
 * SPDX-License-Identifier: Zlib
 */

import Foundation
import SDLSwift
import NeHe
import simd

struct Vertex
{
	var position: SIMD3<Float>, texcoord: SIMD2<Float>

	init(_ position: SIMD3<Float>, _ texcoord: SIMD2<Float>)
	{
		self.position = position
		self.texcoord = texcoord
	}
}

struct Sector
{
	let vertices: [Vertex]

	init?(loadFromURL file: URL)
	{
		// Read world file as ASCII lines
		guard let text = try? String(contentsOf: file, encoding: .ascii) else {	return nil }
		let lines = text
			.components(separatedBy: .newlines)
			.lazy.filter {
				// Skip empty and commented lines
				!$0.trimmingCharacters(in: .whitespacesAndNewlines).isEmpty && $0.first != "/"
			}

		// Read the number of triangles
		var numTriangles = 0
		guard let first = lines.first else { return nil }
		let scanner = Scanner(string: first)
		scanner.scanString("NUMPOLLIES", into: nil)
		guard scanner.scanInt(&numTriangles) else { return nil }

		// Read remaining lines of "%f %f %f %f %f" as triangle list vertices
		var vertices = [Vertex](repeating: .init(.zero, .zero), count: 3 * numTriangles)
		for (i, line) in zip(vertices.indices, lines.dropFirst())
		{
			let scanner = Scanner(string: line)
			var vertex = Vertex(.zero, .zero)
			scanner.scanFloat(&vertex.position.x)  // x
			scanner.scanFloat(&vertex.position.y)  // y
			scanner.scanFloat(&vertex.position.z)  // z
			scanner.scanFloat(&vertex.texcoord.x)  // u
			scanner.scanFloat(&vertex.texcoord.y)  // v
			vertices[i] = vertex
		}

		self.vertices = vertices
	}
}

struct Camera
{
	public var position = SIMD3<Float>(0.0, Self.height, 0.0)
	public var yaw: Float = 0.0, pitch: Float = 0.0

	private var walkBobTheta: Float = 0.0

	mutating func update(_ keys: UnsafePointer<Bool>)
	{
		if keys[Int(SDL_SCANCODE_UP.rawValue)]
		{
			self.position -= self.forward * 0.05
			walkBob(10.0)
		}
		if keys[Int(SDL_SCANCODE_DOWN.rawValue)]
		{
			self.position += self.forward * 0.05
			walkBob(-10.0)
		}
		if keys[Int(SDL_SCANCODE_LEFT.rawValue)]     { self.yaw += 1.0 }
		if keys[Int(SDL_SCANCODE_RIGHT.rawValue)]    { self.yaw -= 1.0 }
		if keys[Int(SDL_SCANCODE_PAGEUP.rawValue)]   { self.pitch -= 1.0 }
		if keys[Int(SDL_SCANCODE_PAGEDOWN.rawValue)] { self.pitch += 1.0 }
	}

	private mutating func walkBob(_ delta: Float)
	{
		if delta.sign == .plus && self.walkBobTheta >= 359.0
		{
			self.walkBobTheta = 0.0
		}
		else if delta.sign == .minus && self.walkBobTheta <= 1.0
		{
			self.walkBobTheta = 359.0
		}
		else
		{
			self.walkBobTheta += delta
		}
		self.position.y = Self.height + sin(self.walkBobTheta * Self.piOver180) / 20.0
	}

	private var forward: SIMD3<Float>
	{
		.init(
			sin(self.yaw * Self.piOver180), 0.0,
			cos(self.yaw * Self.piOver180))
	}

	private static let height: Float = 0.25
	private static let piOver180: Float = 0.0174532925
}

struct Lesson10: AppDelegate
{
	var pso: OpaquePointer? = nil, psoBlend: OpaquePointer? = nil
	var vtxBuffer: OpaquePointer? = nil
	var samplers = [OpaquePointer?](repeating: nil, count: 3)
	var texture: OpaquePointer? = nil

	var projection: simd_float4x4 = .init(1.0)

	var blending = false
	var filter = 0

	var camera = Camera()
	var world: Sector!

	mutating func `init`(ctx: inout NeHeContext) throws(NeHeError)
	{
		guard let worldFile = Bundle.module.url(forResource: "World", withExtension: "txt"),
			let world = Sector(loadFromURL: worldFile) else
		{
			throw .fatalError("Failed to load World.txt")
		}
		self.world = world

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

		var psoInfo = SDL_GPUGraphicsPipelineCreateInfo()
		psoInfo.vertex_shader   = vertexShader
		psoInfo.fragment_shader = fragmentShader
		psoInfo.primitive_type  = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST
		psoInfo.vertex_input_state = SDL_GPUVertexInputState(
			vertex_buffer_descriptions: vertexDescriptions.withUnsafeBufferPointer(\.baseAddress!),
			num_vertex_buffers: UInt32(vertexDescriptions.count),
			vertex_attributes: vertexAttributes.withUnsafeBufferPointer(\.baseAddress!),
			num_vertex_attributes: UInt32(vertexAttributes.count))
		psoInfo.rasterizer_state.fill_mode  = SDL_GPU_FILLMODE_FILL
		psoInfo.rasterizer_state.cull_mode  = SDL_GPU_CULLMODE_NONE
		psoInfo.rasterizer_state.front_face = SDL_GPU_FRONTFACE_COUNTER_CLOCKWISE  // Right-handed coordinates

		// Common pipeline target info setup
		var colorTargets =
		[
			SDL_GPUColorTargetDescription(
				format: SDL_GetGPUSwapchainTextureFormat(ctx.device, ctx.window),
				blend_state: SDL_GPUColorTargetBlendState()),
		]
		psoInfo.target_info.color_target_descriptions = colorTargets.withUnsafeBufferPointer(\.baseAddress!)
		psoInfo.target_info.num_color_targets         = UInt32(colorTargets.count)
		psoInfo.target_info.depth_stencil_format      = ctx.depthTextureFormat
		psoInfo.target_info.has_depth_stencil_target  = true

		// Create blend pipeline (no depth test)
		colorTargets[0].blend_state.enable_blend = true
		colorTargets[0].blend_state.color_blend_op = SDL_GPU_BLENDOP_ADD
		colorTargets[0].blend_state.alpha_blend_op = SDL_GPU_BLENDOP_ADD
		colorTargets[0].blend_state.src_color_blendfactor = SDL_GPU_BLENDFACTOR_SRC_ALPHA
		colorTargets[0].blend_state.dst_color_blendfactor = SDL_GPU_BLENDFACTOR_ONE
		colorTargets[0].blend_state.src_alpha_blendfactor = SDL_GPU_BLENDFACTOR_SRC_ALPHA
		colorTargets[0].blend_state.dst_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE
		guard let psoBlend = SDL_CreateGPUGraphicsPipeline(ctx.device, &psoInfo) else
		{
			throw .sdlError("SDL_CreateGPUGraphicsPipeline", String(cString: SDL_GetError()))
		}
		self.psoBlend = psoBlend

		// Create regular pipeline w/ depth testing
		psoInfo.depth_stencil_state.compare_op         = SDL_GPU_COMPAREOP_LESS
		psoInfo.depth_stencil_state.enable_depth_test  = true
		psoInfo.depth_stencil_state.enable_depth_write = true
		colorTargets[0].blend_state = SDL_GPUColorTargetBlendState()
		guard let pso = SDL_CreateGPUGraphicsPipeline(ctx.device, &psoInfo) else
		{
			throw .sdlError("SDL_CreateGPUGraphicsPipeline", String(cString: SDL_GetError()))
		}
		self.pso = pso

		// Create texture samplers (nearest, linear, and trilinear mipmap)
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
		self.samplers[2] = try createSampler(filter: SDL_GPU_FILTER_LINEAR,
			mipMode: SDL_GPU_SAMPLERMIPMAPMODE_LINEAR,
			maxLod: .greatestFiniteMagnitude)

		// Upload texture and world vertex data
		try ctx.copyPass { (pass) throws(NeHeError) in
			self.texture = try pass.createTextureFrom(bmpResource: "Mud", flip: true, genMipmaps: true)
			self.vtxBuffer = try pass.createBuffer(usage: SDL_GPU_BUFFERUSAGE_VERTEX, self.world.vertices[...])
		}
	}

	func quit(ctx: NeHeContext)
	{
		SDL_ReleaseGPUBuffer(ctx.device, self.vtxBuffer)
		SDL_ReleaseGPUTexture(ctx.device, self.texture)
		self.samplers.reversed().forEach { SDL_ReleaseGPUSampler(ctx.device, $0) }
		SDL_ReleaseGPUGraphicsPipeline(ctx.device, self.pso)
		SDL_ReleaseGPUGraphicsPipeline(ctx.device, self.psoBlend)
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
		colorInfo.clear_color = SDL_FColor(r: 0.0, g: 0.0, b: 0.0, a: 0.0)
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
		SDL_BindGPUGraphicsPipeline(pass, self.blending ? self.psoBlend : self.pso)

		// Bind texture
		var textureBinding = SDL_GPUTextureSamplerBinding(texture: self.texture, sampler: self.samplers[self.filter])
		SDL_BindGPUFragmentSamplers(pass, 0, &textureBinding, 1)

		// Bind world vertex buffer
		let vtxBindings = [ SDL_GPUBufferBinding(buffer: self.vtxBuffer, offset: 0) ]
		SDL_BindGPUVertexBuffers(pass, 0,
			vtxBindings.withUnsafeBufferPointer(\.baseAddress!), UInt32(vtxBindings.count))

		// Setup the camera view matrix
		var modelView = simd_float4x4.rotation(angle: camera.pitch, axis: .init(1.0, 0.0, 0.0))
		modelView.rotate(angle: 360.0 - camera.yaw, axis: .init(0.0, 1.0, 0.0))
		modelView.translate(-camera.position)

		// Push shader uniforms
		var modelViewProj = self.projection * modelView
		SDL_PushGPUVertexUniformData(cmd, 0, &modelViewProj, UInt32(MemoryLayout<simd_float4x4>.size))

		// Draw world
		SDL_DrawGPUPrimitives(pass, UInt32(self.world.vertices.count), 1, 0, 0)

		SDL_EndGPURenderPass(pass)

		// Handle keyboard input
		if let keys = SDL_GetKeyboardState(nil) { camera.update(keys) }
	}

	mutating func key(ctx: inout NeHeContext, key: SDL_Keycode, down: Bool, repeat: Bool)
	{
		guard down && !`repeat` else { return }
		switch key
		{
		case SDLK_B:
			self.blending = !self.blending
		case SDLK_F:
			self.filter = (self.filter + 1) % self.samplers.count
		default:
			break
		}
	}
}

@main struct Program: AppRunner
{
	typealias Delegate = Lesson10
	static let config = AppConfig(
		title: "Lionel Brits & NeHe's 3D World Tutorial",
		width: 640,
		height: 480,
		createDepthBuffer: SDL_GPU_TEXTUREFORMAT_D16_UNORM,
		bundle: Bundle.module)
}
