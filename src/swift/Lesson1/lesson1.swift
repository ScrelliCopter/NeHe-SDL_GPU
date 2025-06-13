/*
 * SPDX-FileCopyrightText: (C) 2025 a dinosaur
 * SPDX-License-Identifier: Zlib
 */

import SDLSwift
import NeHe

struct Lesson1: AppDelegate
{
	func draw(ctx: inout NeHeContext, cmd: OpaquePointer,
		swapchain: OpaquePointer, swapchainSize: Size<UInt32>) throws(NeHeError)
	{
		var colorInfo = SDL_GPUColorTargetInfo()
		colorInfo.texture     = swapchain
		colorInfo.clear_color = SDL_FColor(r: 0.0, g: 0.0, b: 0.0, a: 0.5)
		colorInfo.load_op     = SDL_GPU_LOADOP_CLEAR
		colorInfo.store_op    = SDL_GPU_STOREOP_STORE

		let pass = SDL_BeginGPURenderPass(cmd, &colorInfo, 1, nil)
		SDL_EndGPURenderPass(pass)
	}
}

@main struct Program: AppRunner
{
	typealias Delegate = Lesson1
	static let config = AppConfig(
		title: "NeHe's OpenGL Framework",
		width: 640,
		height: 480)
}
