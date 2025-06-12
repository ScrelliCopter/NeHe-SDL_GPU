/*
 * SPDX-FileCopyrightText: (C) 2025 a dinosaur
 * SPDX-License-Identifier: Zlib
 */

import SDLSwift
import Foundation

public struct NeHeCopyPass: ~Copyable
{
	private var device: OpaquePointer
	private var bundle: Bundle?
	fileprivate var copies: [Copy]

	internal init(_ ctx: borrowing NeHeContext)
	{
		self.device = ctx.device
		self.bundle = ctx.bundle
		self.copies = .init()
	}

	deinit
	{
		// Free transfer buffers
		for copy in self.copies.reversed()
		{
			SDL_ReleaseGPUTransferBuffer(self.device, copy.transferBuffer)
		}
	}

	internal func submit() throws(NeHeError)
	{
		guard let cmd = SDL_AcquireGPUCommandBuffer(self.device) else
		{
			let message = String(cString: SDL_GetError())
			throw .sdlError("SDL_AcquireGPUCommandBuffer", message)
		}

		// Begin the copy pass
		let pass = SDL_BeginGPUCopyPass(cmd)

		// Upload buffers and textures into the GPU buffer(s)
		for copy in self.copies
		{
			switch copy.payload
			{
			case .buffer(let buffer, let size):
				var source = SDL_GPUTransferBufferLocation(transfer_buffer: copy.transferBuffer, offset: 0)
				var destination = SDL_GPUBufferRegion(buffer: buffer, offset: 0, size: size)
				SDL_UploadToGPUBuffer(pass, &source, &destination, false)
			case .texture(let texture, let size, _):
				var source = SDL_GPUTextureTransferInfo()
				source.transfer_buffer = copy.transferBuffer
				source.offset = 0
				var destination = SDL_GPUTextureRegion()
				destination.texture = texture
				destination.w = size.width
				destination.h = size.height
				destination.d = 1  // info.layer_count_or_depth
				SDL_UploadToGPUTexture(pass, &source, &destination, false)
			}
		}

		// End the copy pass
		SDL_EndGPUCopyPass(pass)

		// Generate mipmaps if needed
		for case .texture(let texture, _, let genMipmaps) in self.copies.map(\.payload) where genMipmaps
		{
			SDL_GenerateMipmapsForGPUTexture(cmd, texture)
		}

		SDL_SubmitGPUCommandBuffer(cmd)
	}
}

public extension NeHeCopyPass
{
	mutating func createBuffer<E>(usage: SDL_GPUBufferUsageFlags, _ elements: ArraySlice<E>) throws(NeHeError) -> OpaquePointer
	{
		// Create data buffer
		let size = UInt32(MemoryLayout<E>.stride * elements.count)
		var info = SDL_GPUBufferCreateInfo(usage: usage, size: size, props: 0)
		guard let buffer = SDL_CreateGPUBuffer(self.device, &info) else
		{
			throw .sdlError("SDL_CreateGPUBuffer", String(cString: SDL_GetError()))
		}

		// Create transfer buffer
		var xferInfo = SDL_GPUTransferBufferCreateInfo(
			usage: SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
			size: size,
			props: 0)
		guard let xferBuffer = SDL_CreateGPUTransferBuffer(self.device, &xferInfo) else
		{
			let message = String(cString: SDL_GetError())
			SDL_ReleaseGPUBuffer(self.device, buffer)
			throw .sdlError("SDL_CreateGPUTransferBuffer", message)
		}

		// Map transfer buffer and copy the data
		guard let map = SDL_MapGPUTransferBuffer(self.device, xferBuffer, false) else
		{
			let message = String(cString: SDL_GetError())
			SDL_ReleaseGPUTransferBuffer(self.device, xferBuffer)
			SDL_ReleaseGPUBuffer(self.device, buffer)
			throw .sdlError("SDL_MapGPUTransferBuffer", message)
		}
		elements.withUnsafeBufferPointer
		{
			map.assumingMemoryBound(to: E.self).initialize(from: $0.baseAddress!, count: $0.count)
		}
		SDL_UnmapGPUTransferBuffer(self.device, xferBuffer)

		self.copies.append(.init(
			transferBuffer: xferBuffer,
			payload: .buffer(buffer: buffer, size: size)))
		return buffer
	}

	mutating func createTextureFrom(bmpResource name: String, flip: Bool = false, genMipmaps: Bool = false)
		throws(NeHeError) -> OpaquePointer
	{
		// Load image into a surface
		guard let bundle = self.bundle,
			let pathURL = bundle.url(forResource: name, withExtension: "bmp")
		else
		{
			throw .fatalError("Failed to load BMP resource")
		}
		let path = if #available(macOS 13.0, *) { pathURL.path(percentEncoded: false) } else { pathURL.path }
		guard let image = SDL_LoadBMP(path) else
		{
			throw .sdlError("SDL_LoadBMP", String(cString: SDL_GetError()))
		}
		defer { SDL_DestroySurface(image) }

		// Flip surface if requested
		if flip
		{
			guard SDL_FlipSurface(image, SDL_FLIP_VERTICAL) else
			{
				throw .sdlError("SDL_FlipSurface", String(cString: SDL_GetError()))
			}
		}

		// Upload texture to GPU
		return try self.createTextureFrom(surface: image, genMipmaps: genMipmaps)
	}

	mutating func createTextureFrom(surface: UnsafeMutablePointer<SDL_Surface>,
		genMipmaps: Bool) throws(NeHeError) -> OpaquePointer
	{
		var info = SDL_GPUTextureCreateInfo()
		info.type   = SDL_GPU_TEXTURETYPE_2D
		info.format = SDL_GPU_TEXTUREFORMAT_INVALID
		info.usage  = SDL_GPU_TEXTUREUSAGE_SAMPLER
		info.width  = UInt32(surface.pointee.w)
		info.height = UInt32(surface.pointee.h)
		info.layer_count_or_depth = 1
		info.num_levels           = 1

		let needsConvert: Bool
		(needsConvert, info.format) = switch surface.pointee.format
		{
		case SDL_PIXELFORMAT_RGBA32:        (false, SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM)
		case SDL_PIXELFORMAT_RGBA64:        (false, SDL_GPU_TEXTUREFORMAT_R16G16B16A16_UNORM)
		case SDL_PIXELFORMAT_RGB565:        (false, SDL_GPU_TEXTUREFORMAT_B5G6R5_UNORM)
		case SDL_PIXELFORMAT_ARGB1555:      (false, SDL_GPU_TEXTUREFORMAT_B5G5R5A1_UNORM)
		case SDL_PIXELFORMAT_BGRA4444:      (false, SDL_GPU_TEXTUREFORMAT_B4G4R4A4_UNORM)
		case SDL_PIXELFORMAT_BGRA32:        (false, SDL_GPU_TEXTUREFORMAT_B8G8R8A8_UNORM)
		case SDL_PIXELFORMAT_RGBA64_FLOAT:  (false, SDL_GPU_TEXTUREFORMAT_R16G16B16A16_FLOAT)
		case SDL_PIXELFORMAT_RGBA128_FLOAT: (false, SDL_GPU_TEXTUREFORMAT_R32G32B32A32_FLOAT)
		default:                            (true, SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM)
		}

		let data: UnsafeRawBufferPointer
		let conv: UnsafeMutablePointer<SDL_Surface>? = nil
		if needsConvert
		{
			// Convert pixel format if required
			guard let conv = SDL_ConvertSurface(surface, SDL_PIXELFORMAT_ABGR8888) else
			{
				throw .sdlError("SDL_ConvertSurface", String(cString: SDL_GetError()))
			}
			data = .init(start: conv.pointee.pixels, count: Int(conv.pointee.pitch) * Int(conv.pointee.h))
		}
		else
		{
			data = .init(start: surface.pointee.pixels, count: Int(surface.pointee.pitch) * Int(surface.pointee.h))
		}
		defer { if needsConvert { SDL_DestroySurface(conv) } }

		if genMipmaps
		{
			info.usage |= SDL_GPU_TEXTUREUSAGE_COLOR_TARGET
			// floor(log₂(max(𝑤,ℎ)) + 1
			info.num_levels = 31 - UInt32(max(info.width, info.height).leadingZeroBitCount) + 1
		}

		return try self.createTextureFrom(pixels: data, createInfo: &info, genMipmaps: genMipmaps)
	}
}

fileprivate extension NeHeCopyPass
{
	mutating func createTextureFrom(pixels: UnsafeRawBufferPointer,
		createInfo info: inout SDL_GPUTextureCreateInfo, genMipmaps: Bool)
		throws(NeHeError) -> OpaquePointer
	{
		guard let texture = SDL_CreateGPUTexture(self.device, &info) else
		{
			throw .sdlError("SDL_CreateGPUTexture", String(cString: SDL_GetError()))
		}

		// Create a transfer buffer to hold the pixel data
		var xferInfo = SDL_GPUTransferBufferCreateInfo(
			usage: SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
			size: UInt32(pixels.count),
			props: 0)
		guard let xferBuffer = SDL_CreateGPUTransferBuffer(self.device, &xferInfo) else
		{
			SDL_ReleaseGPUTexture(self.device, texture)
			throw .sdlError("SDL_CreateGPUTransferBuffer", String(cString: SDL_GetError()))
		}

		// Copy the image pixel data to a transfer buffer
		guard let map = SDL_MapGPUTransferBuffer(self.device, xferBuffer, false) else
		{
			SDL_ReleaseGPUTransferBuffer(self.device, xferBuffer)
			SDL_ReleaseGPUTexture(self.device, texture)
			throw .sdlError("SDL_MapGPUTransferBuffer", String(cString: SDL_GetError()))
		}
		map.initializeMemory(as: UInt8.self,
			from: pixels.withMemoryRebound(to: UInt8.self, \.baseAddress!),
			count: pixels.count)
		SDL_UnmapGPUTransferBuffer(self.device, xferBuffer)

		self.copies.append(.init(
			transferBuffer: xferBuffer,
			payload: .texture(
				texture: texture,
				size: .init(info.width, info.height),
				genMipmaps: genMipmaps)))
		return texture
	}
}

fileprivate extension NeHeCopyPass
{
	struct Copy
	{
		enum Payload
		{
			case buffer(buffer: OpaquePointer, size: UInt32)
			case texture(texture: OpaquePointer, size: Size<UInt32>, genMipmaps: Bool)
		}

		let transferBuffer: OpaquePointer
		let payload: Payload
	}
}
