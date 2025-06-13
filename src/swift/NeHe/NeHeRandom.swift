/*
 * SPDX-FileCopyrightText: (C) 2025 a dinosaur
 * SPDX-License-Identifier: Zlib
 */

public struct NeHeRandom: Equatable
{
	var state: UInt32

	public init()
	{
		self.state = 1
	}

	public init(seed: UInt32)
	{
		self.state = seed
	}

	public mutating func next() -> Int32
	{
		self.state = self.state &* 214013 &+ 2531011
		return Int32(bitPattern: (self.state >> 16) & 0x7FFF)  // (s / 65536) % 32768
	}
}
