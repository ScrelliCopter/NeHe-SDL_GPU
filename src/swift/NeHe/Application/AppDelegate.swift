/*
 * SPDX-FileCopyrightText: (C) 2025 a dinosaur
 * SPDX-License-Identifier: Zlib
 */

import SDLSwift

public protocol AppDelegate: ~Copyable
{
	init()
	mutating func `init`(ctx: inout NeHeContext) throws(NeHeError)
	mutating func quit(ctx: NeHeContext)
	mutating func resize(size: Size<Int32>)
	mutating func draw(ctx: inout NeHeContext, cmd: OpaquePointer,
		swapchain: OpaquePointer, swapchainSize: Size<UInt32>) throws(NeHeError)
	mutating func key(ctx: inout NeHeContext, key: SDL_Keycode, down: Bool, repeat: Bool)
}

public extension AppDelegate
{
	mutating func `init`(ctx: inout NeHeContext) throws(NeHeError) {}
	mutating func quit(ctx: NeHeContext) {}
	mutating func resize(size: Size<Int32>) {}
	mutating func key(ctx: inout NeHeContext, key: SDL_Keycode, down: Bool, repeat: Bool) {}
}
