/*
 * SPDX-FileCopyrightText: (C) 2025 a dinosaur
 * SPDX-License-Identifier: Zlib
 */

use core::ops::Mul;

#[derive(PartialEq, Clone, Copy)]
pub struct Mtx([f32; 16]);

#[allow(dead_code)]
impl Mtx
{
	pub(crate) const fn new(
		m00: f32, m01: f32, m02: f32, m03: f32,
		m10: f32, m11: f32, m12: f32, m13: f32,
		m20: f32, m21: f32, m22: f32, m23: f32,
		m30: f32, m31: f32, m32: f32, m33: f32) -> Self
	{
		Self([
			m00, m01, m02, m03,
			m10, m11, m12, m13,
			m20, m21, m22, m23,
			m30, m31, m32, m33,
		])
	}

	pub const IDENTITY: Self = Self::new(
		0.0, 0.0, 0.0, 0.0,
		0.0, 1.0, 0.0, 0.0,
		0.0, 0.0, 1.0, 0.0,
		0.0, 0.0, 0.0, 1.0);

	pub const fn translation(x: f32, y: f32, z: f32) -> Self
	{
		Self::new(
			1.0, 0.0, 0.0, 0.0,
			0.0, 1.0, 0.0, 0.0,
			0.0, 0.0, 1.0, 0.0,
			  x,   y,   z, 1.0)
	}

	const fn make_rotation(c: f32, s: f32, x: f32, y: f32, z: f32) -> [f32; 9]
	{
		let rc = 1.0 - c;
		let (rcx, rcy, rcz) = (x * rc, y * rc, z * rc);
		let (sx, sy, sz) = (x * s, y * s, z * s);

		[
			rcx * x +  c, rcy * x + sz, rcz * x - sy,
			rcx * y - sz, rcy * y +  c, rcz * y + sx,
			rcx * z + sy, rcy * z - sx, rcz * z +  c,
		]
	}

	fn make_gl_rotation(angle: f32, x: f32, y: f32, z: f32) -> [f32; 9]
	{
		// Treat inputs like glRotatef
		let theta = angle.to_radians();
		let axis_mag = (x * x + y * y + z * z).sqrt();
		let (nx, ny, nz) =
			if (axis_mag - 1.0).abs() > f32::EPSILON
			{ (x / axis_mag, y / axis_mag, z / axis_mag) } else
			{ (x, y, z) };
		Self::make_rotation(theta.cos(), theta.sin(), nx, ny, nz)
	}

	pub(crate) fn rotation(angle: f32, x: f32, y: f32, z: f32) -> Self
	{
		let r = Self::make_gl_rotation(angle, x, y, z);
		Self::new(
			r[0], r[1], r[2], 0.0,
			r[3], r[4], r[5], 0.0,
			r[6], r[7], r[8], 0.0,
			 0.0,  0.0,  0.0, 1.0)
	}

	pub const fn perspective(fovy: f32, aspect: f32, near: f32, far: f32) -> Self
	{
		let h = 1.0 / (fovy.to_radians() * 0.5);
		let w = h / aspect;
		let invcliprng = 1.0 / (far - near);
		let zh = -(far + near) * invcliprng;
		let zl = -(2.0 * far * near) * invcliprng;

		/*
		  [w  0  0  0]
		  [0  h  0  0]
		  [0  0 zh zl]
		  [0  0 -1  0]
		*/
		Self::new(
			  w, 0.0, 0.0,  0.0,
			0.0,   h, 0.0,  0.0,
			0.0, 0.0,  zh, -1.0,
			0.0, 0.0,  zl,  0.0)
	}

	pub const fn translate(&mut self, x: f32, y: f32, z: f32)
	{
		/*
		  m = { [1 0 0 x]
		        [0 1 0 y]
		        [0 0 1 z]
		        [0 0 0 1] } * m
		*/
		let m = self.0;
		self.0[12] += x * m[0] + y * m[4] + z * m[8];
		self.0[13] += x * m[1] + y * m[5] + z * m[9];
		self.0[14] += x * m[2] + y * m[6] + z * m[10];
		self.0[15] += x * m[3] + y * m[7] + z * m[11];
	}

	pub fn rotate(&mut self, angle: f32, x: f32, y: f32, z: f32)
	{
		let r = Self::make_gl_rotation(angle, x, y, z);

		// Partial matrix multiplication
		let mut tmp = [0f32; 12];  // Set up temporary
		tmp.copy_from_slice(&self.0[..12]);
		self.0[0]  = r[0] * tmp[0] + r[1] * tmp[4] + r[2] * tmp[8];
		self.0[1]  = r[0] * tmp[1] + r[1] * tmp[5] + r[2] * tmp[9];
		self.0[2]  = r[0] * tmp[2] + r[1] * tmp[6] + r[2] * tmp[10];
		self.0[3]  = r[0] * tmp[3] + r[1] * tmp[7] + r[2] * tmp[11];
		self.0[4]  = r[3] * tmp[0] + r[4] * tmp[4] + r[5] * tmp[8];
		self.0[5]  = r[3] * tmp[1] + r[4] * tmp[5] + r[5] * tmp[9];
		self.0[6]  = r[3] * tmp[2] + r[4] * tmp[6] + r[5] * tmp[10];
		self.0[7]  = r[3] * tmp[3] + r[4] * tmp[7] + r[5] * tmp[11];
		self.0[8]  = r[6] * tmp[0] + r[7] * tmp[4] + r[8] * tmp[8];
		self.0[9]  = r[6] * tmp[1] + r[7] * tmp[5] + r[8] * tmp[9];
		self.0[10] = r[6] * tmp[2] + r[7] * tmp[6] + r[8] * tmp[10];
		self.0[11] = r[6] * tmp[3] + r[7] * tmp[7] + r[8] * tmp[11];
	}

	#[inline(always)]
	#[must_use]
	pub const fn as_ptr(&self) -> *const f32 { self.0.as_ptr() }
}

impl Default for Mtx
{
	fn default() -> Self { Self::IDENTITY }
}

impl Mul for Mtx
{
	type Output = Self;
	fn mul(self, rhs: Self) -> Self
	{
		Self(core::array::from_fn(|i|
		{
			let (row, col) = (i & 0x3, i >> 2);
			(0..4).fold(0f32, |a, j| a +
				self.0[j * 4 + row] *
				rhs.0[col * 4 + j])
		}))
	}
}
