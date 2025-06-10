/*
 * SPDX-FileCopyrightText: (C) 2025 a dinosaur
 * SPDX-License-Identifier: Zlib
 */

import SDLSwift

public protocol AppRunner
{
	associatedtype Delegate: AppDelegate
	static var config: AppConfig { get }
}

public extension AppRunner
{
	static func main() throws -> Void
	{
		// Initialise SDL
		guard SDL_Init(SDL_INIT_VIDEO) else
		{
			throw NeHeError.sdlError("SDL_Init", String(cString: SDL_GetError()))
		}
		defer { SDL_Quit() }

		// Initialise GPU context
		var ctx = try NeHeContext(
			title: config.title,
			width: config.width,
			height: config.height,
			bundle: config.bundle)
		defer
		{
			SDL_ReleaseWindowFromGPUDevice(ctx.device, ctx.window)
			SDL_DestroyGPUDevice(ctx.device)
			SDL_DestroyWindow(ctx.window)
		}

		// Handle depth buffer texture creation if requested
		if config.manageDepthFormat != SDL_GPU_TEXTUREFORMAT_INVALID
		{
			var backbufWidth: Int32 = 0, backbufHeight: Int32 = 0
			SDL_GetWindowSizeInPixels(ctx.window, &backbufWidth, &backbufHeight)
			try ctx.setupDepthTexture(
				size: .init(UInt32(backbufWidth), UInt32(backbufHeight)),
				format: config.manageDepthFormat)
		}
		defer
		{
			if config.manageDepthFormat != SDL_GPU_TEXTUREFORMAT_INVALID,
				let depthTexture = ctx.depthTexture
			{
				SDL_ReleaseGPUTexture(ctx.device, depthTexture)
			}
		}

		// Initialise app delegate
		var app = Delegate()
		try app.`init`(ctx: &ctx)

		var fullscreen = false

		// Enter main loop
		quit: while true
		{
			// Process events
			var event = SDL_Event()
			while SDL_PollEvent(&event)
			{
				switch SDL_EventType(event.type)
				{
				case SDL_EVENT_QUIT:
					break quit
				case SDL_EVENT_WINDOW_ENTER_FULLSCREEN, SDL_EVENT_WINDOW_LEAVE_FULLSCREEN:
					fullscreen = event.type == SDL_EVENT_WINDOW_ENTER_FULLSCREEN.rawValue
				case SDL_EVENT_KEY_DOWN:
					if event.key.key == SDLK_ESCAPE
					{
						break quit
					}
					if event.key.key == SDLK_F1
					{
						SDL_SetWindowFullscreen(ctx.window, !fullscreen)
						break
					}
					fallthrough
				case SDL_EVENT_KEY_UP:
					app.key(ctx: &ctx, key: event.key.key, down: event.key.down, repeat: event.key.repeat)
				case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED:
					app.resize(size: .init(width: event.window.data1, height: event.window.data2))
				default:
					break
				}
			}

			guard let cmd = SDL_AcquireGPUCommandBuffer(ctx.device) else
			{
				throw NeHeError.sdlError("SDL_AcquireGPUCommandBuffer", String(cString: SDL_GetError()))
			}

			var swapchainTexture: OpaquePointer?, swapchainWidth: Int32 = 0, swapchainHeight: Int32 = 0
			guard SDL_WaitAndAcquireGPUSwapchainTexture(cmd, ctx.window,
				&swapchainTexture, &swapchainWidth, &swapchainHeight) else
			{
				let message = String(cString: SDL_GetError())
				SDL_CancelGPUCommandBuffer(cmd)
				throw NeHeError.sdlError("SDL_WaitAndAcquireGPUSwapchainTexture", message)
			}

			guard let swapchain = swapchainTexture else
			{
				SDL_CancelGPUCommandBuffer(cmd)
				continue
			}

			let swapchainSize = Size(width: UInt32(swapchainWidth), height: UInt32(swapchainHeight))

			if config.manageDepthFormat != SDL_GPU_TEXTUREFORMAT_INVALID
				&& ctx.depthTexture != nil
				&& ctx.depthTextureSize != swapchainSize
			{
				try ctx.setupDepthTexture(size: swapchainSize)
			}

			try app.draw(ctx: &ctx, cmd: cmd, swapchain: swapchain, swapchainSize: swapchainSize)
			SDL_SubmitGPUCommandBuffer(cmd)
		}
	}
}
