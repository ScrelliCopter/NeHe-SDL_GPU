#!/usr/bin/env python3
"""
SPDX-FileCopyrightText: (C) 2025 a dinosaur
SPDX-License-Identifier: Zlib
"""
import os
import sys
from abc import ABC, abstractmethod
from collections import namedtuple, defaultdict
from dataclasses import dataclass
from pathlib import Path
from string import Template
from typing import List, TextIO, NamedTuple, Tuple, Iterable, Type


class Shader(NamedTuple):
	source: Path
	output: Path
	definitions: frozenset[str] | None

	def with_output_suffix(self, out_suffix: str, /):
		"""Return the shader with a suffix appended to the output

		:param out_suffix: File extension to append to output
		:return:           The modified shader
		"""
		return Shader(self.source,
			self.output.with_name(f"{self.output.name}.{out_suffix}"),
			self.definitions)

	def definition_flags(self, flag: str = "-D") -> str | None:
		"""Get the shader's defines as flags

		:param flag: Override the prepended flag (defaults to "-D")
		:return:     List of flags
		"""
		if self.definitions is None:
			return None
		return " ".join(f"{flag}{d}" for d in self.definitions)


def shaders_suffixes(shaders: Iterable[Shader],
		in_suffix: str | None, out_suffixes: Iterable[str] | str | None = None) -> Iterable[Shader]:
	"""Add file extensions to the source and outputs of a list of shaders

	:param shaders:      The list of Shader tuples
	:param in_suffix:    Optional file extension to append to source
	:param out_suffixes: Optional file extension(s) to append to output
	:return:             Generator with the modified shaders
	"""
	import collections.abc
	if not isinstance(out_suffixes, str) and isinstance(out_suffixes, collections.abc.Iterable):
		for s in shaders:
			for out_suffix in out_suffixes:
				yield Shader(
					Path(f"{s.source}.{in_suffix}") if in_suffix else s.source,
					Path(f"{s.output}.{out_suffix}"),
					s.definitions)
	else:
		for s in shaders:
			yield Shader(
				Path(f"{s.source}.{in_suffix}") if in_suffix else s.source,
				Path(f"{s.output}.{out_suffixes}") if out_suffixes else s.output,
				s.definitions)


class Pattern(NamedTuple):
	folder: str | Path | None
	suffix: str


class Rule(NamedTuple):
	prerequisite: Pattern
	target: Pattern
	recipe: List[str]


@dataclass
class Configure:
	macros: dict[str, str]
	meta: dict[str, List[Shader]]
	rules: List[Rule]


class Dependency(NamedTuple):
	source: str
	target: str
	recipe: List[str]
	definitions: str | None


class Writer(ABC):
	@classmethod
	def __init__(self, file: TextIO, configure: Configure, /):
		self.out = file
		self.configure = configure

	@abstractmethod
	def generate(self):
		pass

	@staticmethod
	def pattern_string(name: str, pat: Pattern, /) -> str:
		if pat.folder is not None:
			return f"{str(pat.folder).rstrip('/') or ''}/{name}{pat.suffix}"
		else:
			return f"{name}{pat.suffix}"

	@staticmethod
	def match_pattern(p: Path, pat: Pattern, /) -> bool:
		if not "".join(p.suffixes).endswith(pat.suffix):
			return False
		if pat.folder is None:
			if len(p.parts) > 1:
				return False
		elif not p.is_relative_to(pat.folder):
			return False
		return True

	def unroll_dependencies(self) -> Iterable[Tuple[str, List[Dependency]]]:
		def unroll_targets(self, targets: List[Shader], /) -> Iterable[Dependency]:
			for target in targets:
				if rule := next((rule for rule in self.configure.rules
						if self.match_pattern(target.output, rule.target)
						and self.match_pattern(target.source, rule.prerequisite)), None):
					yield Dependency(str(target.source), str(target.output), rule.recipe, target.definition_flags())
				else:
					rule = next(rule for rule in self.configure.rules if self.match_pattern(target.output, rule.target))
					name = target.output.stem
					output = str(target.output)
					while not self.match_pattern(target.source, rule.prerequisite):
						source = str(Path(rule.prerequisite.folder, name).with_suffix(rule.prerequisite.suffix))
						yield Dependency(source, output, rule.recipe, None)
						rule = next(next_rule for next_rule in self.configure.rules
							if rule.prerequisite == next_rule.target)
						output = source
					yield Dependency(str(target.source), output, rule.recipe, target.definition_flags())

		for group, targets in self.configure.meta.items():
			yield group, list(unroll_targets(self, targets))


class MakefileWriter(Writer):
	@classmethod
	def write_specials(self):
		pass

	@abstractmethod
	def write_rules(self, macro_map: dict[str, str], /):
		pass

	def write_dependency(self, dependency: Dependency, macro_map: dict[str, str], explicit: bool, /):
		def write_rule(predicate: str, target: str, recipe: List[str], macro_map: dict[str, str], /):
			print(f"{target}: {predicate}", file=self.out)
			for line in recipe:
				print("\t" + Template(line).substitute(macro_map), file=self.out)
			print(file=self.out)
		write_rule(dependency.source, dependency.target, dependency.recipe, {
			"source": dependency.source if explicit else "$<",
			"target": dependency.target if explicit else "$@",
			"definitions": "" if dependency.definitions is None else dependency.definitions,
			**macro_map})

	def generate(self):
		macro_map = {}
		if len(self.configure.macros):
			left_pad = max(len(x) for x in self.configure.macros.keys())
			for name, value in self.configure.macros.items():
				name_upper = name.upper()
				macro_map.update({f"macro_{name}": f"$({name_upper})"})
				print(f"{name_upper:<{left_pad}} := {value}", file=self.out)
			print(file=self.out)

		self.write_specials()
		meta_targets = self.configure.meta.keys()
		print(f"all:", *meta_targets, file=self.out)
		print(".PHONY:", "all", *meta_targets, "clean", file=self.out)

		for target, prerequisites in self.configure.meta.items():
			print(f"{target}:", *(i.output for i in prerequisites), file=self.out)
		print(file=self.out)

		self.dependencies = dict(self.unroll_dependencies())
		self.write_rules(macro_map)

		print("clean:", file=self.out)
		for group in self.dependencies.values():
			print(f"\trm -f", *(x[1] for x in group), file=self.out)


class PosixMakefileWriter(MakefileWriter):
	def write_specials(self):
		print(".POSIX:", file=self.out)
		print(".SUFFIXES:", file=self.out)

	def write_rules(self, macro_map: dict[str, str], /):
		for group in self.dependencies.values():
			for dependency in group:
				self.write_dependency(dependency, macro_map, True)


class GnuMakefileWriter(MakefileWriter):
	def write_rules(self, macro_map: dict[str, str], /):
		for group in self.dependencies.values():
			for dependency in group:
				if dependency.definitions:
					self.write_dependency(dependency, macro_map, False)
		for rule in self.configure.rules:
			self.write_dependency(Dependency(
					self.pattern_string("%", rule.prerequisite),
					self.pattern_string("%", rule.target),
					rule.recipe, ""),
				macro_map, False)


class Environment(NamedTuple):
	writer: Type[Writer]
	make_name: str
	src_dir: Path
	dest_dir: Path
	is_darwin: bool
	is_windows: bool
	metal_debug: bool


Groups = namedtuple("Groups", ["metal", "glsl", "hlsl"])


def find_shaders(root: Path, e: Environment, /) -> Groups:
	from configparser import ConfigParser
	config = ConfigParser()
	config.read(root / e.src_dir / "shaders.ini")

	metal_shaders: set[Shader] = set()
	glsl_shaders: set[Shader] = set()
	hlsl_shaders: set[Shader] = set()

	for line in config.items("Shaders"):
		tokens = line[1].split()

		source = tokens[0]
		output = line[0]
		definitions = tokens[1:]
		definitions = frozenset(definitions) if len(definitions) else None

		source_path = e.src_dir / source
		output_path = e.dest_dir / output
		msl_source = root.joinpath(source_path).with_name(f"{source}.metal")
		glsl_source = root.joinpath(source_path).with_name(f"{source}.glsl")
		hlsl_source = root.joinpath(source_path).with_name(f"{source}.hlsl")
		msl_exists = msl_source.is_file()
		glsl_exists = glsl_source.is_file()
		hlsl_exists = hlsl_source.is_file()

		if not msl_exists and not glsl_exists and not hlsl_exists:
			sys.exit(f"FATAL: \"{source}\" specified in shaders.ini but no corresponding metal, glsl, or hlsl exists")

		if msl_exists:
			if not glsl_exists and not hlsl_exists:
				print(f"WARN: \"{msl_source.name}\" exists but no \"{glsl_source.name}\" or \"{hlsl_source.name}\"")
		elif glsl_exists:
			print(f"WARN: \"{glsl_source.name}\" exists but no \"{msl_source.name}\"")
		elif hlsl_exists:
			print(f"WARN: \"{hlsl_source.name}\" exists but no \"{msl_source.name}\"")
		if glsl_exists and not hlsl_exists:
			print(f"WARN: \"{glsl_source.name}\" exists but no \"{hlsl_source.name}\"")

		if e.is_darwin and msl_exists:
			metal_shaders.add(Shader(source_path, output_path, definitions))
		if glsl_exists:
			glsl_shaders.add(Shader(source_path, output_path, definitions))
		if hlsl_exists:
			hlsl_shaders.add(Shader(source_path, output_path, definitions))

	return Groups(metal_shaders, glsl_shaders, hlsl_shaders)


def compile_recipe(shader_groups: Groups, e: Environment) -> Configure:
	hlsl_spirv = shader_groups.hlsl - shader_groups.glsl
	use_glslang = bool(shader_groups.glsl)
	use_dxc_spirv = bool(hlsl_spirv)
	use_metal = e.is_darwin and bool(shader_groups.metal)
	use_dxc = bool(shader_groups.hlsl)
	use_fxb = e.is_windows and use_dxc

	configure = Configure({}, defaultdict(list), [])

	# Make rules for SPIR-V shaders for Vulkan
	if use_glslang:
		configure.macros.update({
			"glslang": "glslang",
		})
		configure.meta["vulkan"] += \
			list(shaders_suffixes(shader_groups.glsl, "glsl", ["vtx.spv", "frg.spv"]))
		configure.rules += [
			Rule(
				Pattern(e.src_dir, ".glsl"),
				Pattern(e.dest_dir, ".vtx.spv"),
				["$macro_glslang --quiet -V -S vert -DVERTEX -o $target $source"]),
			Rule(
				Pattern(e.src_dir, ".glsl"),
				Pattern(e.dest_dir, ".frg.spv"),
				["$macro_glslang --quiet -V -S frag -o $target $source"]),
		]

	if use_dxc or use_dxc_spirv:
		configure.macros.update({
			"dxc": "dxc",
		})

	if use_dxc_spirv:
		configure.meta["vulkan"] += \
			list(shaders_suffixes(hlsl_spirv, "hlsl", ["vtx.spv", "frg.spv"]))
		configure.rules += [
			Rule(
				Pattern(e.src_dir, ".hlsl"),
				Pattern(e.dest_dir, ".vtx.spv"),
				["$macro_dxc -spirv -E VertexMain -T vs_6_0 -DVULKAN -DVERTEX $definitions -Fo $target $source"]),
			Rule(
				Pattern(e.src_dir, ".hlsl"),
				Pattern(e.dest_dir, ".frg.spv"),
				["$macro_dxc -spirv -E FragmentMain -T ps_6_0 -DVULKAN $definitions -Fo $target $source"]),
		]

	# Make rules for Metal shaders on macOS
	if use_metal:
		configure.macros.update({
			"metal_platform": "macos",
			"metal_sdk": "macosx",
			"metal_version_min": "10.11",
			"metalc": "$(shell xcrun -sdk $(METAL_SDK) -f metal)"
		})
		if e.metal_debug:
			configure.macros.update({"metal_dsymutil": "$(shell xcrun -sdk $(METAL_SDK) -f metal-dsymutil)"})
		else:
			configure.macros.update({"metalld": "$(shell xcrun -sdk $(METAL_SDK) -f metallib)"})
		configure.macros.update({
			"metalflags": " ".join([
				"-Wall",
				"-O3",
				"-std=$(METAL_PLATFORM)-metal1.1",
				"-m$(METAL_SDK)-version-min=$(METAL_VERSION_MIN)",
				"-frecord-sources"
			])
		})
		configure.meta["metal"] += \
			list(shaders_suffixes(shader_groups.metal, "metal", "metallib"))
		configure.rules += [
			Rule(
				Pattern(e.src_dir, ".metal"),
				Pattern(e.dest_dir, ".air"),
				["$macro_metalc $macro_metalflags $definitions -c -o $target $source"]),
			Rule(
				Pattern(e.dest_dir, ".air"),
				Pattern(e.dest_dir, ".metallib"), [
					"$macro_metalc -frecord-sources -o $target $source",
					"$macro_metal_dsymutil -flat -remove-source $target"
				]) if e.metal_debug else
			Rule(
				Pattern(e.dest_dir, ".air"),
				Pattern(e.dest_dir, ".metallib"),
				["$macro_metalld -o $target $source"]),
		]

	# Make rules for HLSL shaders on Windows
	if use_dxc:
		configure.meta["d3d12"] += \
			list(shaders_suffixes(shader_groups.hlsl, "hlsl", ["vtx.dxb", "pxl.dxb"]))
		configure.rules += [
			Rule(
				Pattern(e.src_dir, ".hlsl"),
				Pattern(e.dest_dir, ".vtx.dxb"),
				["$macro_dxc -E VertexMain -T vs_6_0 -DD3D12 -DVERTEX $definitions -Fo $target $source"]),
			Rule(
				Pattern(e.src_dir, ".hlsl"),
				Pattern(e.dest_dir, ".pxl.dxb"),
				["$macro_dxc -E PixelMain -T ps_6_0 -DD3D12 $definitions -Fo $target $source"]),
		]

	# FXC is only available through the Windows SDK
	if use_fxb:
		configure.macros.update({
			"fxc": "fxc",
		})
		configure.meta["d3d12"] += \
			list(shaders_suffixes(shader_groups.hlsl, "hlsl", ["vtx.fxb", "pxl.fxb"]))
		configure.rules += [
			Rule(
				Pattern(e.src_dir, ".hlsl"),
				Pattern(e.dest_dir, ".vtx.fxb"),
				["$macro_fxc /E VertexMain /T vs_5_1 /DD3D12 /DVERTEX $definitions /Fo $target $source"]),
			Rule(
				Pattern(e.src_dir, ".hlsl"),
				Pattern(e.dest_dir, ".pxl.fxb"),
				["$macro_fxc /E PixelMain /T ps_5_1 /DD3D12 $definitions /Fo $target $source"]),
		]

	return configure


def build_makefiles(basedir: Path, /):
	root = basedir.parent
	src_dir = Path("src/shaders")
	dest_dir = Path("data/shaders")

	ini_stat = os.stat(root / src_dir / "shaders.ini")

	def generate_makefile(writer: Type[Writer], name: str, is_darwin: bool, is_windows: bool, /):
		makefile_stat = os.stat(basedir / "shaders" / name)
		if makefile_stat.st_mtime >= ini_stat.st_mtime:
			return

		e = Environment(writer, name, src_dir, dest_dir, is_darwin, is_windows, False)
		with basedir.joinpath("shaders", e.make_name).open("w") as out:
			e.writer(out, compile_recipe(find_shaders(root, e), e)).generate()

	generate_makefile(GnuMakefileWriter, "Makefile.darwin", True, False)
	generate_makefile(PosixMakefileWriter, "Makefile.unix", False, False)


def run_makefile(basedir: Path, /):
	import shutil
	import subprocess
	import os
	import platform

	root = basedir.parent

	# Try to find cross-platform shader compilers
	defs = {
		"glslang": shutil.which("glslang"),
		"dxc": shutil.which("dxc", path=f"/opt/dxc/bin:{os.environ.get('PATH', os.defpath)}"),
	}
	if (system := platform.system()) == "Darwin":
		sdk_platform = "macosx"
		def xcrun_find_program(name: str):
			return subprocess.run(["xcrun", "-sdk", sdk_platform, "-f", name],
				capture_output=True, check=True).stdout.decode("utf-8").strip()
		defs.update({
			"metalc": xcrun_find_program("metal"),
			"metalld": xcrun_find_program("metallib"),
			"metal_platform": "macos",
			"metal_sdk": sdk_platform,
			"metal_version_min": "10.11"
		})
		makefile = "Makefile.darwin"
	elif system == "Windows":
		pass
	else:
		makefile = "Makefile.unix"

	make_path = basedir.joinpath("shaders", makefile).relative_to(root)
	make_cmd = ["make", "-f", str(make_path), *(f"{k.upper()}={v}" for k, v in defs.items())]
	subprocess.run(make_cmd, cwd=root, check=True)


def main():
	basedir = Path(sys.argv[0]).resolve().parent
	build_makefiles(basedir)
	run_makefile(basedir)


if __name__ == "__main__":
	main()
