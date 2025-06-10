/*
 * SPDX-FileCopyrightText: (C) 2025 a dinosaur
 * SPDX-License-Identifier: Zlib
 */

import SDLSwift
import Foundation

public struct AppConfig: Sendable
{
	let title: StaticString
	let width: Int32
	let height: Int32
	let manageDepthFormat: SDL_GPUTextureFormat

	let bundle: Bundle?

	public init(title: StaticString, width: Int32, height: Int32,
		createDepthBuffer depthFormat: SDL_GPUTextureFormat? = nil,
		bundle: Bundle? = nil)
	{
		self.title = title
		self.width = width
		self.height = height
		self.manageDepthFormat = depthFormat ?? SDL_GPU_TEXTUREFORMAT_INVALID
		self.bundle = bundle
	}
}
