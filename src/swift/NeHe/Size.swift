/*
 * SPDX-FileCopyrightText: (C) 2025 a dinosaur
 * SPDX-License-Identifier: Zlib
 */

public struct Size<T: Numeric>: Equatable
{
	public var width: T, height: T

	public init(width: T, height: T)
	{
		self.width = width
		self.height = height
	}

	@inline(__always) public init(_ w: T, _ h: T) { self.init(width: w, height: h) }
}

public extension Size
{
	@inline(__always) static var zero: Self { Self(0, 0) }
}

public extension Size where T: BinaryInteger
{
	init<O: BinaryInteger>(_ other: Size<O>)
	{
		self.init(T(other.width), T(other.height))
	}
}
