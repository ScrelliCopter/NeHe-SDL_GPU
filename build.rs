/*
 * SPDX-FileCopyrightText: (C) 2025 a dinosaur
 * SPDX-License-Identifier: Zlib
 */

use std::path::{Path, PathBuf};

pub fn get_source_dir() -> PathBuf
{
	std::env::current_dir().unwrap().join("src")
}

pub fn get_target_dir() -> PathBuf
{
	//let out_dir = std::env::var("OUT_DIR").unwrap();
	let manifest_dir = std::env::var("CARGO_MANIFEST_DIR").unwrap();
	let build_type = std::env::var("PROFILE").unwrap();
	Path::new(&manifest_dir).join("target").join(build_type)
}

pub fn copy_resources<const N: usize>(src_dir: &PathBuf, dst_dir: &PathBuf, resources: &[&str; N])
{
	if !dst_dir.is_dir()
	{
		std::fs::create_dir(&dst_dir).unwrap();
	}
	for resource in resources
	{
		std::fs::copy(&src_dir.join(resource), &dst_dir.join(resource)).unwrap();
	}
}

pub fn main()
{
	#[cfg(target_os="macos")]
	println!("cargo:rustc-link-arg=-Wl,-rpath,/Library/Frameworks");

	let src_dir = std::env::current_dir().unwrap().join("data");
	let dst_dir = get_target_dir().join("Data");

	copy_resources(&src_dir, &dst_dir,
	&[
		"NeHe.bmp",
		"Crate.bmp",
		"Glass.bmp",
		"Star.bmp",
		"Mud.bmp",
		"World.txt",
	]);
	copy_resources(&src_dir.join("shaders"), &dst_dir.join("Shaders"),
	&[
		"lesson2.metallib",
		"lesson3.metallib",
		"lesson6.metallib",
		"lesson7.metallib",
		"lesson9.metallib",
	]);
}
