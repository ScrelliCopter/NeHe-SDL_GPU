#!/usr/bin/env python3
"""
SPDX-FileCopyrightText: (C) 2025 a dinosaur
SPDX-License-Identifier: Zlib
"""

import os
import shutil
import sys
from collections import namedtuple
from pathlib import Path
import subprocess
import platform
from typing import Iterable


def compile_metal_shaders(
		sources: list[str | Path], library: str | Path,
		cflags: list[str] = None, sdk="macosx", debug: bool = False, cwd: Path | None = None) -> None:
	"""Build a Metal shader library from a list of Metal shaders

	:param sources: List of Metal shader source paths
	:param library: Path to the Metal library to build
	:param cflags:  Optional list of additional compiler parameters
	:param sdk:     Name of the Xcode platform SDK to use (default: "macosx"),
	                can be "macosx", "iphoneos", "iphonesimulator", "appletvos", or "appletvsimulator"
	:param debug:   Generate a symbol file that can be used to debug the shader library
	:param cwd:     Optionally set the current working directory for the compiler
	"""
	if cflags is None:
		cflags = []

	def xcrun_find_program(name: str):
		return subprocess.run(["xcrun", "-sdk", sdk, "-f", name],
			capture_output=True, check=True).stdout.decode("utf-8").strip()

	# Find Metal compilers
	metal = xcrun_find_program("metal")
	if debug:
		metal_dsymutil = xcrun_find_program("metal-dsymutil")
	else:
		metallib = xcrun_find_program("metallib")

	# Compile each source to an AIR (Apple Intermediate Representation)
	cflags = cflags + ["-frecord-sources"]
	air_objects = [f"{str(s).removesuffix('.metal')}.air" for s in sources]
	for src, obj in zip(sources, air_objects):
		subprocess.run([metal, *cflags, "-c", src, "-o", obj], cwd=cwd, check=True)

	try:
		# Build the Metal library
		if debug:
			subprocess.run([metal, "-frecord-sources", "-o", library, *air_objects], cwd=cwd, check=True)
			subprocess.run([metal_dsymutil, "-flat", "-remove-source", library], cwd=cwd, check=True)
		else:
			subprocess.run([metallib, "-o", library, *air_objects], cwd=cwd, check=True)
	finally:
		# Clean up AIR objects
		for obj in air_objects:
			cwd.joinpath(obj).unlink()


Shader = namedtuple("Shader", ["source", "type", "output"])


def shaders_suffixes(shaders: list[Shader],
		in_suffix: str | None, out_suffix: str | None) -> Iterable[Shader]:
	"""Add file extensions to the source and outputs of a list of shaders

	:param shaders:    The list of Shader tuples
	:param in_suffix:  Optional file extension to append to source
	:param out_suffix: Optional file extension to append to output
	:return:           Generator with the modified shaders
	"""
	for s in shaders:
		yield Shader(
			f"{s.source}.{in_suffix}" if in_suffix else s.source,
			s.type,
			f"{s.output}.{out_suffix}" if out_suffix else s.output)


def compile_spirv_shaders(shaders: Iterable[Shader],
		flags: list[str] = None, glslang: str | None=None, cwd: Path | None = None) -> None:
	"""Compile shaders to SPIR-V using glslang

	:param shaders: The list of shader source paths to compile
	:param flags:   List of additional flags to pass to glslang
	:param glslang: Optional path to glslang executable, if `None` defaults to "glslang"
	:param cwd:     Optionally set the current working directory for the compiler
	"""
	if glslang is None:
		glslang = "glslang"
	if flags is None:
		flags = []

	for shader in shaders:
		sflags = [*flags, "-V", "-S", shader.type, "-o", shader.output, shader.source]
		subprocess.run([glslang, *sflags], cwd=cwd, check=True)


def compile_dxil_shaders(shaders: Iterable[Shader], dxc: str | None = None, cwd: Path | None = None) -> None:
	"""Compile HLSL shaders to DXIL using DXC

	:param shaders: The list of shader source paths to compile
	:param dxc:     Optional path to dxc excutable, if `None` defaults to "dxc"
	:param cwd:     Optionally set the current working directory for the compiler
	"""
	if dxc is None:
		dxc = "dxc"
	for shader in shaders:
		entry, shader_type = {
			"vert": ("VertexMain", "vs_6_0"),
			"frag": ("PixelMain", "ps_6_0") }[shader.type]
		cflags = ["-E", entry, "-T", shader_type]
		subprocess.run([dxc, *cflags, "-Fo", shader.output, shader.source], cwd=cwd, check=True)


def compile_dxbc_shaders(shaders: Iterable[Shader], cwd: Path | None = None) -> None:
	"""Compile HLSL shaders to DXBC using FXC

	:param shaders: The list of shader source paths to compile
	:param cwd:     Optionally set the current working directory for the compiler
	"""
	for shader in shaders:
		entry, shader_type = {
			"vert": ("VertexMain", "vs_5_1"),
			"frag": ("PixelMain", "ps_5_1") }[shader.type]
		cflags = ["/E", entry, "/T", shader_type]
		subprocess.run(["fxc", *cflags, "/Fo", shader.output, shader.source], cwd=cwd, check=True)


def compile_shaders() -> None:
	build_spirv = False
	build_metal = True
	build_dxil = False
	build_dxbc = False
	lessons = [ "lesson2", "lesson3", "lesson6", "lesson7", "lesson9" ]
	src_dir = Path("src/shaders")
	dest_dir = Path("data/shaders")

	system = platform.system()
	def add_shaders() -> Iterable[Shader]:
		for lesson in lessons:
			yield Shader(src_dir / f"{lesson}.vertex", "vert", dest_dir / f"{lesson}.vertex")
			yield Shader(src_dir / f"{lesson}.fragment", "frag", dest_dir / f"{lesson}.fragment")
	shaders = list(add_shaders())

	root = Path(sys.argv[0]).resolve().parent.parent
	root.joinpath(dest_dir).mkdir(parents=True, exist_ok=True)

	# Try to find cross-platform shader compilers
	glslang = shutil.which("glslang")
	dxc = shutil.which("dxc", path=f"/opt/dxc/bin:{os.environ.get('PATH', os.defpath)}")

	if build_spirv:
		# Build SPIR-V shaders for Vulkan
		compile_spirv_shaders(shaders_suffixes(shaders, "glsl", "spv"),
			flags=["--quiet"], glslang=glslang, cwd=root)

	if build_metal:
		# Build Metal shaders on macOS
		if system == "Darwin":
			compile_platform = "macos"
			sdk_platform = "macosx"
			min_version = "10.11"
			for lesson in lessons:
				compile_metal_shaders(
					sources=[src_dir / f"{lesson}.metal"],
					library=dest_dir / f"{lesson}.metallib",
					cflags=["-Wall", "-O3",
						f"-std={compile_platform}-metal1.1",
						f"-m{sdk_platform}-version-min={min_version}"],
					sdk=sdk_platform,
					cwd=root)

	if build_dxil:
		# Build HLSL shaders on Windows or when DXC is available
		if system == "Windows" or dxc is not None:
			compile_dxil_shaders(shaders_suffixes(shaders, "hlsl", "dxb"), dxc=dxc, cwd=root)
	if build_dxbc:
		if system == "Windows":  # FXC is only available thru the Windows SDK
			compile_dxbc_shaders(shaders_suffixes(shaders, "hlsl", "fxb"), cwd=root)


if __name__ == "__main__":
	compile_shaders()
