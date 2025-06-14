/*
 * SPDX-FileCopyrightText: (C) 2025 a dinosaur
 * SPDX-License-Identifier: Zlib
 */

pub struct Random(u32);

impl Random
{
	#[inline(always)]
	pub fn new() -> Self { Self(1) }
	#[inline(always)]
	pub fn new_seed(seed: u32) -> Self { Self(seed) }

	#[inline(always)]
	#[must_use]
	pub fn seed(&self) -> u32 { self.0 }
	#[inline(always)]
	pub fn set_seed(&mut self, seed: u32)
	{
		self.0 = seed
	}

	pub fn next(&mut self) -> i32
	{
		self.0 = self.0.wrapping_mul(214013).wrapping_add(2531011);
		((self.0 >> 16) & 0x7FFF) as i32  // (s / 65536) % 32768
	}
}

impl Default for Random
{
	#[inline(always)]
	fn default() -> Self { Self::new() }
}
