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

pub fn copy_resources<S: AsRef<str>>(src_dir: &PathBuf, dst_dir: &PathBuf, resources: Vec<S>)
{
	if !dst_dir.is_dir()
	{
		std::fs::create_dir(&dst_dir).unwrap();
	}
	for resource in resources
	{
		std::fs::copy(&src_dir.join(resource.as_ref()), &dst_dir.join(resource.as_ref())).unwrap();
	}
}

pub fn copy_shaders<const N: usize>(src_dir: &PathBuf, dst_dir: &PathBuf, shaders: &[&str; N])
{
	let resources = shaders.into_iter().flat_map(|name|
	{
		if cfg!(target_os="macos")
		{
			vec![format!("{name}.metallib")]
		}
		else
		{
			vec![format!("{name}.vtx.spv"), format!("{name}.frg.spv")]
		}
	}).collect();
	copy_resources(src_dir, dst_dir, resources);
}

pub fn main()
{
	#[cfg(target_os="macos")]
	println!("cargo:rustc-link-arg=-Wl,-rpath,/Library/Frameworks");

	#[cfg(target_os="linux")]
	println!("cargo:rustc-link-arg=-Wl,-rpath,$ORIGIN");

	let src_dir = std::env::current_dir().unwrap().join("data");
	let dst_dir = get_target_dir().join("Data");

	copy_resources(&src_dir, &dst_dir,
	vec![
		"NeHe.bmp",
		"Crate.bmp",
		"Glass.bmp",
		"Star.bmp",
		"Mud.bmp",
		"World.txt",
	]);
	copy_shaders(&src_dir.join("shaders"), &dst_dir.join("Shaders"),
	&[
		"lesson2",
		"lesson3",
		"lesson6",
		"lesson7",
		"lesson8",
		"lesson9",
	]);
}
