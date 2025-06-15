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


Shader = namedtuple("Shader", ["source", "output"])


def shaders_suffixes(shaders: Iterable[Shader],
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
			f"{s.output}.{out_suffix}" if out_suffix else s.output)


def shader_prepend_ext(shader: Shader, prep_ext: str) -> Shader:
	"""Prepend `prep_ext` to the shader's output path's extension

	:param shader:   Shader to modify
	:param prep_ext: Extension to prepend (without the dot)
	:return:         The modified shader
	"""
	out = Path(shader.output)
	return Shader(shader.source, out.with_suffix(f".{prep_ext}{out.suffix}"))


def compile_spirv_shaders(shaders: Iterable[Shader], suffix: str = "spv",
		glslang: str | None = None, dxc: str | None = None, cwd: Path | None = None) -> None:
	"""Compile shaders to SPIR-V using glslang for GLSL shaders and DXC for HLSL shaders

	If a GLSL version exists it will be built with glslang,
	else building HLSL with DXC will be attempted.

	:param shaders: The list of shader source paths to compile
	:param glslang: Optional path to glslang executable, if `None` defaults to "glslang"
	:param dxc:     Optional path to dxc excutable, if `None` defaults to "dxc"
	:param cwd:     Optionally set the current working directory for the compiler
	"""
	for glsl, hlsl in zip(
			shaders_suffixes(shaders, "glsl", suffix),
			shaders_suffixes(shaders, "hlsl", suffix)):
		if cwd.joinpath(glsl.source).exists():
			flags = ["--quiet"]
			compile_glsl_spirv_shader(shader_prepend_ext(glsl, "vtx"), "vert",
				flags=[*flags, "-DVERTEX"], glslang=glslang, cwd=cwd)
			compile_glsl_spirv_shader(shader_prepend_ext(glsl, "frg"), "frag",
				flags=flags, glslang=glslang, cwd=cwd)
		else:
			compile_hlsl_spirv_shader(shader_prepend_ext(hlsl, "vtx"), "vert",
				flags=["-DVULKAN", "-DVERTEX"], dxc=dxc, cwd=cwd)
			compile_hlsl_spirv_shader(shader_prepend_ext(hlsl, "frg"), "frag",
				flags=["-DVULKAN"], dxc=dxc, cwd=cwd)


def compile_glsl_spirv_shader(shader: Shader, type: str, flags: list[str] = None,
		glslang: str | None = None, cwd: Path | None = None) -> None:
	"""Compile GLSL shaders to SPIR-V using glslang

	:param shader:  The shaders to compile
	:param type:    Type of shader to compile
	:param flags:   List of additional flags to pass to glslang
	:param glslang: Optional path to glslang executable, if `None` defaults to "glslang"
	:param cwd:     Optionally set the current working directory for the compiler
	"""
	if glslang is None:
		glslang = "glslang"
	if flags is None:
		flags = []
	flags += ["-V", "-S", type, "-o", shader.output, shader.source]
	subprocess.run([glslang, *flags], cwd=cwd, check=True)


def compile_hlsl_spirv_shader(shader: Shader, type: str, flags: list[str] = None,
		dxc: str | None = None, cwd: Path | None = None) -> None:
	"""Compile HLSL shaders to SPIR-V using DXC

	:param shader: The shaders to compile
	:param type:   Type of shader to compile
	:param flags:   List of additional flags to pass to DXC
	:param dxc:    Optional path to dxc excutable, if `None` defaults to "dxc"
	:param cwd:    Optionally set the current working directory for the compiler
	"""
	if dxc is None:
		dxc = "dxc"
	if flags is None:
		flags = []
	entry, shader_type = {
		"vert": ("VertexMain", "vs_6_0"),
		"frag": ("FragmentMain", "ps_6_0") }[type]
	cflags = ["-spirv", *flags, "-E", entry, "-T", shader_type]
	subprocess.run([dxc, *cflags, "-Fo", shader.output, shader.source], cwd=cwd, check=True)


def compile_d3d12_shaders(shaders: Iterable[Shader], build_dxbc: bool = False,
		dxc: str | None = None, cwd: Path | None = None) -> None:
	for shader in shaders_suffixes(shaders, "hlsl", "dxb"):
		compile_dxil_shader(shader_prepend_ext(shader, "vtx"), "vert",
			flags=["-DD3D12", "-DVERTEX"], dxc=dxc, cwd=cwd)
		compile_dxil_shader(shader_prepend_ext(shader, "pxl"), "frag",
			flags=["-DD3D12"], dxc=dxc, cwd=cwd)
	if build_dxbc:  # FXC is only available thru the Windows SDK
		for shader in shaders_suffixes(shaders, "hlsl", "fxb"):
			compile_dxbc_shader(shader_prepend_ext(shader, "vtx"), "vert",
				flags=["/DD3D12", "/DVERTEX"], cwd=cwd)
			compile_dxbc_shader(shader_prepend_ext(shader, "pxl"), "frag",
				flags=["/DD3D12"], cwd=cwd)


def compile_dxil_shader(shader: Shader, type: str, flags: list[str] | None = None,
		dxc: str | None = None, cwd: Path | None = None) -> None:
	"""Compile an HLSL shaders to DXIL using DXC

	:param shader: Path to the shader source to compile
	:param type:   Type of shader to compile
	:param flags:  Optional list of flags to pass to DXC
	:param dxc:    Optional path to dxc excutable, if `None` defaults to "dxc"
	:param cwd:    Optionally set the current working directory for the compiler
	"""
	if flags is None:
		flags = []
	if dxc is None:
		dxc = "dxc"
	entry, shader_type = {
		"vert": ("VertexMain", "vs_6_0"),
		"frag": ("PixelMain", "ps_6_0") }[type]
	cflags = [*flags, "-E", entry, "-T", shader_type]
	subprocess.run([dxc, *cflags, "-Fo", shader.output, shader.source], cwd=cwd, check=True)


def compile_dxbc_shader(shader: Shader, type: str, flags: list[str] | None = None,
		cwd: Path | None = None) -> None:
	"""Compile an HLSL shader to DXBC using FXC

	:param shader: Path to the shader source to compile
	:param type:   Type of shader to compile
	:param flags:  Optional list of flags to pass to FXC
	:param cwd:    Optionally set the current working directory for the compiler
	"""
	if flags is None:
		flags = []
	entry, shader_type = {
		"vert": ("VertexMain", "vs_5_1"),
		"frag": ("PixelMain", "ps_5_1") }[type]
	cflags = [*flags, "/E", entry, "/T", shader_type]
	subprocess.run(["fxc", *cflags, "/Fo", shader.output, shader.source], cwd=cwd, check=True)


def compile_shaders() -> None:
	build_spirv = True
	build_metal = True
	build_dxil = True
	build_dxbc = False
	lessons = [ "lesson2", "lesson3", "lesson6", "lesson7", "lesson8", "lesson9" ]
	src_dir = Path("src/shaders")
	dest_dir = Path("data/shaders")

	system = platform.system()
	shaders = [Shader(src_dir / lesson, dest_dir / lesson) for lesson in lessons]

	root = Path(sys.argv[0]).resolve().parent.parent
	root.joinpath(dest_dir).mkdir(parents=True, exist_ok=True)

	# Try to find cross-platform shader compilers
	glslang = shutil.which("glslang")
	dxc = shutil.which("dxc", path=f"/opt/dxc/bin:{os.environ.get('PATH', os.defpath)}")

	if build_spirv:
		# Build SPIR-V shaders for Vulkan
		compile_spirv_shaders(shaders, glslang=glslang, dxc=dxc, cwd=root)

	if build_metal:
		# Build Metal shaders on macOS
		if system == "Darwin":
			compile_platform = "macos"
			sdk_platform = "macosx"
			min_version = "10.11"
			for shader in shaders_suffixes(shaders, "metal", "metallib"):
				compile_metal_shaders(
					sources=[shader.source],
					library=shader.output,
					cflags=["-Wall", "-O3",
						f"-std={compile_platform}-metal1.1",
						f"-m{sdk_platform}-version-min={min_version}"],
					sdk=sdk_platform,
					cwd=root)

	# Build HLSL shaders on Windows or when DXC is available
	is_windows = system == "Windows"
	if (build_dxil or (is_windows and build_dxbc)) and (is_windows or dxc is not None):
		compile_d3d12_shaders(shaders, build_dxbc=build_dxbc and is_windows, dxc=dxc, cwd=root)


if __name__ == "__main__":
	compile_shaders()
