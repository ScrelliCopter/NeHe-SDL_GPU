/*
 * SPDX-FileCopyrightText: (C) 2025 a dinosaur
 * SPDX-License-Identifier: Zlib
 */

import simd

public extension simd_float4x4
{
	static let identity: Self = matrix_identity_float4x4

	@inlinable static func translation(_ position: SIMD3<Float>) -> Self
	{
		Self(
		.init( 1, 0, 0, 0),
		.init( 0, 1, 0, 0),
		.init( 0, 0, 1, 0),
		.init(position, 1))
	}

	static func rotation(angle: Float, axis: SIMD3<Float>) -> Self
	{
		let r = simd_float3x3.makeGLRotation(angle, axis)
		return Self(
			.init(r.columns.0, 0),
			.init(r.columns.1, 0),
			.init(r.columns.2, 0),
			.init( 0,  0,  0,  1))
	}

	static func perspective(fovy: Float, aspect: Float, near: Float, far: Float) -> Self
	{
		let h = 1 / tan(fovy * (.pi / 180) * 0.5)
		let w = h / aspect
		let invClipRng = 1 / (far - near)
		let zh = -(far + near) * invClipRng
		let zl = -(2 * far * near) * invClipRng

		return Self(
			.init(w, 0,  0,  0),
			.init(0, h,  0,  0),
			.init(0, 0, zh, -1),
			.init(0, 0, zl,  0))
	}

	@inlinable mutating func translate(_ offset: SIMD3<Float>)
	{
		/*
		  m = { [1 0 0 x]
		        [0 1 0 y]
		        [0 0 1 z]
		        [0 0 0 1] } * m
		*/
		self.columns.3 +=
			offset.x * self.columns.0 +
			offset.y * self.columns.1 +
			offset.z * self.columns.2
	}

	mutating func rotate(angle: Float, axis: SIMD3<Float>)
	{
		let r = simd_float3x3.makeGLRotation(angle, axis)

		// Set up temporaries
		let (t0, t1, t2) = (self.columns.0, self.columns.1, self.columns.2)

		// Partial matrix multiplication
		self.columns.0 = r.columns.0.x * t0 + r.columns.0.y * t1 + r.columns.0.z * t2
		self.columns.1 = r.columns.1.x * t0 + r.columns.1.y * t1 + r.columns.1.z * t2
		self.columns.2 = r.columns.2.x * t0 + r.columns.2.y * t1 + r.columns.2.z * t2
	}
}

fileprivate extension simd_float3x3
{
	static func makeRotation(_ c: Float, _ s: Float, _ axis: SIMD3<Float>) -> Self
	{
		let rc = 1 - c
		let rcv = rc * axis, sv = s * axis
		return Self(
			rcv * axis.x + .init(   +c, +sv.z, -sv.y),
			rcv * axis.y + .init(-sv.z,    +c, +sv.x),
			rcv * axis.z + .init(+sv.y, -sv.x,    +c))
	}

	static func makeGLRotation(_ angle: Float, _ axis: SIMD3<Float>) -> Self
	{
		// Treat inputs like glRotatef
		let theta = angle * (.pi / 180)
		let axisMag = simd_length(axis)
		let n = (abs(axisMag - 1) > .ulpOfOne)
			? axis / axisMag
			: axis
		return Self.makeRotation(cos(theta), sin(theta), n)
	}
}
