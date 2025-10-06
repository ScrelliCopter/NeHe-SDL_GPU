/*
 * SPDX-FileCopyrightText: (C) 2025 a dinosaur
 * SPDX-License-Identifier: Zlib
 */

import SDLSwift
import Foundation

public struct NeHeContext
{
	public var window: OpaquePointer
	public var device: OpaquePointer
	public var depthTexture: OpaquePointer?
	public var depthTextureSize: Size<UInt32>
	public let depthTextureFormat: SDL_GPUTextureFormat
	public let bundle: Bundle?
}

internal extension NeHeContext
{
	init(config: AppConfig) throws(NeHeError)
	{
		// Create window
		let flags = SDL_WindowFlags(SDL_WINDOW_RESIZABLE) | SDL_WindowFlags(SDL_WINDOW_HIGH_PIXEL_DENSITY)
		guard let window = SDL_CreateWindow(config.title.utf8Start, config.width, config.height, flags) else
		{
			throw .sdlError("SDL_CreateWindow", String(cString: SDL_GetError()))
		}

		// Open GPU device
		let formats =
			SDL_GPU_SHADERFORMAT_METALLIB | SDL_GPU_SHADERFORMAT_MSL |
			SDL_GPU_SHADERFORMAT_SPIRV | SDL_GPU_SHADERFORMAT_DXIL
		guard let device = SDL_CreateGPUDevice(formats, true, nil) else
		{
			throw .sdlError("SDL_CreateGPUDevice", String(cString: SDL_GetError()))
		}

		// Attach window to the GPU device
		guard SDL_ClaimWindowForGPUDevice(device, window) else
		{
			throw .sdlError("SDL_ClaimWindowForGPUDevice", String(cString: SDL_GetError()))
		}

		// Enable VSync
		SDL_SetGPUSwapchainParameters(device, window,
			SDL_GPU_SWAPCHAINCOMPOSITION_SDR,
			SDL_GPU_PRESENTMODE_VSYNC)

		self.window = window
		self.device = device
		self.depthTexture = nil
		self.depthTextureSize = .zero
		self.depthTextureFormat = config.manageDepthFormat
		self.bundle = config.bundle
	}
}

public extension NeHeContext
{
	mutating func setupDepthTexture(size: Size<UInt32>, format: SDL_GPUTextureFormat, clearDepth: Float = 1.0)
		throws(NeHeError)
	{
		if self.depthTexture != nil
		{
			SDL_ReleaseGPUTexture(self.device, self.depthTexture)
			self.depthTexture = nil
		}

		let props = SDL_CreateProperties()
		guard props != 0 else
		{
			throw .sdlError("SDL_CreateProperties", String(cString: SDL_GetError()))
		}
		// Workaround for https://github.com/libsdl-org/SDL/issues/10758
		SDL_SetFloatProperty(props, SDL_PROP_GPU_TEXTURE_CREATE_D3D12_CLEAR_DEPTH_FLOAT, clearDepth)

		var info = SDL_GPUTextureCreateInfo(
			type: SDL_GPU_TEXTURETYPE_2D,
			format: format,
			usage: SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET,
			width: size.width,
			height: size.height,
			layer_count_or_depth: 1,
			num_levels: 1,
			sample_count: SDL_GPU_SAMPLECOUNT_1,
			props: props)
		let newTexture = SDL_CreateGPUTexture(self.device, &info)
		SDL_DestroyProperties(props)
		guard let texture = newTexture else
		{
			throw .sdlError("SDL_CreateGPUTexture", String(cString: SDL_GetError()))
		}
		self.depthTexture = texture
		self.depthTextureSize = size
	}

	func loadShaders(name: String, vertexUniforms: UInt32 = 0, vertexStorage: UInt32 = 0, fragmentSamplers: UInt32 = 0)
		throws(NeHeError) -> (vertex: OpaquePointer, fragment: OpaquePointer)
	{
		guard let bundle = self.bundle else { throw .fatalError("No bundle") }

		var info = ShaderProgramCreateInfo(format: SDL_GPUShaderFormat(SDL_GPU_SHADERFORMAT_INVALID),
			vertexUniforms: vertexUniforms,
			vertexStorage: vertexStorage,
			fragmentSamplers: fragmentSamplers)

		let availableFormats = SDL_GetGPUShaderFormats(self.device)
		if availableFormats & (SDL_GPU_SHADERFORMAT_METALLIB | SDL_GPU_SHADERFORMAT_MSL) != 0
		{
			if availableFormats & SDL_GPU_SHADERFORMAT_METALLIB == SDL_GPU_SHADERFORMAT_METALLIB
			{
				if let path = bundle.url(forResource: name, withExtension: "metallib"),
					let lib = try? Data(contentsOf: path)
				{
					info.format = SDL_GPU_SHADERFORMAT_METALLIB
					return (
						try loadShaderBlob(lib, info, SDL_GPU_SHADERSTAGE_VERTEX, "VertexMain"),
						try loadShaderBlob(lib, info, SDL_GPU_SHADERSTAGE_FRAGMENT, "FragmentMain"))
				}
			}
			if availableFormats & SDL_GPU_SHADERFORMAT_MSL == SDL_GPU_SHADERFORMAT_MSL
			{
				guard let path = bundle.url(forResource: name, withExtension: "metal"),
					let src = try? Data(contentsOf: path) else
				{
					throw .fatalError("Failed to load metal shader")
				}
				info.format = SDL_GPU_SHADERFORMAT_MSL
				return (
					try loadShaderBlob(src, info, SDL_GPU_SHADERSTAGE_VERTEX, "VertexMain"),
					try loadShaderBlob(src, info, SDL_GPU_SHADERSTAGE_FRAGMENT, "FragmentMain"))
			}
		}
		else if availableFormats & SDL_GPU_SHADERFORMAT_SPIRV == SDL_GPU_SHADERFORMAT_SPIRV
		{
			info.format = SDL_GPU_SHADERFORMAT_SPIRV
			return (
				try loadShader(bundle.url(forResource: name, withExtension: "vtx.spv"),
					info, SDL_GPU_SHADERSTAGE_VERTEX, "VertexMain"),
				try loadShader(bundle.url(forResource: name, withExtension: "frg.spv"),
					info, SDL_GPU_SHADERSTAGE_FRAGMENT, "FragmentMain"))
		}
		else if availableFormats & SDL_GPU_SHADERFORMAT_DXIL == SDL_GPU_SHADERFORMAT_DXIL
		{
			info.format = SDL_GPU_SHADERFORMAT_DXIL
			return (
				try loadShader(bundle.url(forResource: name, withExtension: "vtx.dxb"),
					info, SDL_GPU_SHADERSTAGE_VERTEX, "VertexMain"),
				try loadShader(bundle.url(forResource: name, withExtension: "pxl.dxb"),
					info, SDL_GPU_SHADERSTAGE_FRAGMENT, "PixelMain"))
			
		}
		throw .fatalError("No supported shader formats found")
	}

	func copyPass(pass closure: (inout NeHeCopyPass) throws(NeHeError) -> Void) throws(NeHeError)
	{
		var pass = NeHeCopyPass(self)
		try closure(&pass)
		try pass.submit()
	}
}

fileprivate extension NeHeContext
{
	struct ShaderProgramCreateInfo
	{
		var format: SDL_GPUShaderFormat
		var vertexUniforms: UInt32
		var vertexStorage: UInt32
		var fragmentSamplers: UInt32
	}

	func loadShader(_ filePath: URL?, _ info: borrowing ShaderProgramCreateInfo,
		_ type: SDL_GPUShaderStage, _ main: StaticString) throws(NeHeError) -> OpaquePointer
	{
		guard let path = filePath, let data = try? Data(contentsOf: path) else
		{
			throw .fatalError("Failed to open shader file")
		}
		return try loadShaderBlob(data, info, type, main)
	}

	func loadShaderBlob(_ code: Data, _ info: ShaderProgramCreateInfo,
		_ type: SDL_GPUShaderStage, _ main: StaticString) throws(NeHeError) -> OpaquePointer
	{
		guard let result = code.withContiguousStorageIfAvailable({ data in
			main.withUTF8Buffer { $0.withMemoryRebound(to: Int8.self) { entryPoint in
				var info = SDL_GPUShaderCreateInfo(
					code_size: code.count,
					code: data.baseAddress!,
					entrypoint: entryPoint.baseAddress!,
					format: info.format,
					stage: type,
					num_samplers: type == SDL_GPU_SHADERSTAGE_FRAGMENT ? info.fragmentSamplers : 0,
					num_storage_textures: 0,
					num_storage_buffers: type == SDL_GPU_SHADERSTAGE_VERTEX ? info.vertexStorage : 0,
					num_uniform_buffers: type == SDL_GPU_SHADERSTAGE_VERTEX ? info.vertexUniforms : 0,
					props: 0)
				return SDL_CreateGPUShader(self.device, &info)
			}}
		}) else
		{
			throw .fatalError("Failed to convert shader blob to contiguous storage")
		}
		guard let shader = result else
		{
			throw .sdlError("SDL_CreateGPUShader", String(cString: SDL_GetError()))
		}
		return shader
	}
}

fileprivate extension URL
{
	func join(_ pathComponent: String, isDirectory dir: Bool? = nil) -> URL
	{
		if #available(macOS 13, *)
		{
			let hint: DirectoryHint = dir == nil ? .inferFromPath : (dir! ? .isDirectory : .notDirectory)
			return self.appending(path: pathComponent, directoryHint: hint)
		}
		else
		{
			return dir == nil
				? self.appendingPathComponent(pathComponent)
				: self.appendingPathComponent(pathComponent, isDirectory: dir!)
		}
	}
}
