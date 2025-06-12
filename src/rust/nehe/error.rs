/*
 * SPDX-FileCopyrightText: (C) 2025 a dinosaur
 * SPDX-License-Identifier: Zlib
 */

use std::{fmt, io};
use std::error::Error;
use std::ffi::CStr;
use sdl3_sys::error::SDL_GetError;

#[derive(Debug)]
pub enum NeHeError
{
	Fatal(&'static str),
	SDLError(&'static str, &'static CStr),
	IOError(io::Error),
}

impl NeHeError
{
	pub fn sdl(fname: &'static str) -> Self
	{
		unsafe { NeHeError::SDLError(fname, CStr::from_ptr(SDL_GetError())) }
	}
}

impl fmt::Display for NeHeError
{
	fn fmt(&self, fmt: &mut fmt::Formatter<'_>) -> fmt::Result
	{
		match self
		{
			NeHeError::Fatal(msg) => fmt.write_str(msg),
			NeHeError::SDLError(a, b) => write!(fmt, "{}: {}", a, b.to_string_lossy()),
			NeHeError::IOError(e) => write!(fmt, "std::io: {}", e),
		}
	}
}

impl From<io::Error> for NeHeError
{
	fn from(err: io::Error) -> Self { Self::IOError(err) }
}

impl Error for NeHeError {}
