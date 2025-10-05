#!/usr/bin/env python3
"""
SPDX-FileCopyrightText: (C) 2025 a dinosaur
SPDX-License-Identifier: Zlib
"""

import tomllib
from pathlib import Path
from string import Template
from typing import Iterable, List


class Options:
	depth_texfmt_suffixes = {
		16: "D16_UNORM", 24: "D24_UNORM", 32: "D32_FLOAT",
		248: "D24_UNORM_S8_UINT", 328: "D32_FLOAT_S8_UINT", 48: "D32_FLOAT_S8_UINT"
	}

	def __init__(self):
		from argparse import ArgumentParser
		p = ArgumentParser()
		p.add_argument("num", type=int)
		p.add_argument("--depth", type=int)
		p.add_argument("--key", action="store_true")
		p.add_argument("--projection", action="store_true")
		p.add_argument("--title", default="")
		g = p.add_mutually_exclusive_group()
		g.add_argument("--lesson1", "--minimal", action="store_true")
		g.add_argument("--lesson6", action="store_true")
		g.add_argument("--lesson7", action="store_true")
		l = p.add_mutually_exclusive_group()
		l.add_argument("--c", action="store_true")
		l.add_argument("--rust", action="store_true")
		l.add_argument("--swift", action="store_true")
		a = p.parse_args()

		from datetime import datetime
		self.copyright_year = datetime.now().year
		self.lesson_num = a.num
		depth = a.depth or (16 if a.lesson6 or a.lesson7 else None)
		self.depthfmt_suffix = None if depth is None else f"{self.depth_texfmt_suffixes[depth]}"
		self.key = a.key or a.lesson7
		self.projection = a.projection or a.lesson6 or a.lesson7
		self.title = a.title
		if a.lesson1:
			self.template_name = "lesson1"
		elif a.lesson6:
			self.template_name = "lesson6"
		elif a.lesson7:
			self.template_name = "lesson7"
		else:
			self.template_name = "default"
		if a.rust:
			self.lang = "rust"
		elif a.swift:
			self.lang = "swift"
		else:
			self.lang = "c"


class Value:
	def __init__(self, value, /, comment: str|None = None):
		self.value = value
		self.comment = comment

class SizeOf:
	def __init__(self, struct: str):
		self.struct = struct

class ArraySizeOf:
	def __init__(self, array: str):
		self.array = array

class OffsetOf:
	def __init__(self, struct: str, field: str):
		self.struct = struct
		self.field = field

class Array:
	def __init__(self, elem_type: str, elements: List[dict]):
		self.type = elem_type
		self.elements = elements


class SourceGenerator:
	depth_info = {
		"texture": "ctx->depthTexture",
		"clear_depth": Value(1.0, comment="Ensure depth buffer clears to furthest value"),
		"load_op": "SDL_GPU_LOADOP_CLEAR",
		"store_op": "SDL_GPU_STOREOP_DONT_CARE",
		"stencil_load_op": "SDL_GPU_LOADOP_DONT_CARE",
		"stencil_store_op": "SDL_GPU_STOREOP_DONT_CARE",
		"cycle": True
	}

	def __init__(self, lang_name: str, template_dir: Path):
		with template_dir.joinpath(f"{lang_name}.toml").open("rb") as f:
			lang_toml = tomllib.load(f)

		self.source_dir = lang_toml["source_dir"]
		self.file_extension = lang_toml["file_extension"]
		self.swift_subdir = lang_toml.get("swift_subdir", False)
		self.conditions = lang_toml["condition"]
		self._globals = lang_toml.get("globals", "\n$fields\n\n")
		self._global_field = lang_toml["global_field"]
		self._empty_globals_semicolon = ";" if lang_toml.get("empty_globals_semicolon", False) else ""
		self._appconfig_depthfmt = lang_toml["appconfig_depthfmt"]
		self._depth_info_ref = lang_toml["depth_info_ref"]
		self._null = lang_toml["null"]
		self._matrix_type = lang_toml.get("matrix_type", "Mtx")
		self._struct_depth = lang_toml["struct_depth"]
		self.func_resize_projection = lang_toml["func_resize_projection"]
		self.func_keys = lang_toml["func_keys"]

	@staticmethod
	def sanitise_string(s: str, /) -> str:
		def cstr_chr(c: str) -> str:
			if m := {
					"\a": "\\a", "\b": "\\b", "\f": "\\f", "\n": "\\n", "\r": "\\r",
					"\t": "\\t", "\v": "\\v", "\\": "\\\\", "\"": "\\\"" }.get(c):
				return m
			elif (i := ord(c)) < 0x7F:
				return c if c.isprintable() else f"\\x{i:0X2}"
			elif i < 0x10000:
				return f"\\u{i:04X}"
			else:
				return f"\\U{i:08X}"
		return "".join(cstr_chr(c) for c in s)

	def global_field(self, label: str, field_type: str, /) -> str:
		return Template(self._globals).substitute({
			"fields": Template(self._global_field).substitute({"label": label, "field_type": field_type}),
		})

	def initialiser_field(self, key, value, /, last: bool = False) -> Iterable[str]:
		if isinstance(value, Value):
			finalval = value
		elif isinstance(value, SizeOf):
			finalval = Value(f"sizeof({value.struct})")
		elif isinstance(value, ArraySizeOf):
			finalval = Value(f"SDL_arraysize({value.array})")
		elif isinstance(value, OffsetOf):
			finalval = Value(f"offsetof({value.struct}, {value.field})")
		elif isinstance(value, Array):
			yield f".{key} = &(const {value.type})"
			yield "{"
			if len(value.elements) == 1:
				for line in self.initialiser_fields(value.elements[0]):
					yield line
			yield "}" if last else "},"
			return
		elif isinstance(value, dict):
			yield f".{key} ="
			yield "{"
			for line in self.initialiser_fields(value):
				yield line
			yield "}" if last else "},"
			return
		else:
			finalval = Value(value)
		if isinstance(finalval.value, bool):
			strval = str(finalval.value).lower()
		elif isinstance(finalval.value, float):
			strval = f"{finalval.value}f"
		else:
			strval = str(finalval.value)
		yield f".{key} = {strval}{'' if last else ','}{'  // ' + finalval.comment if finalval.comment else ''}"

	def initialiser_fields(self, info: dict, /) -> Iterable[str]:
		for i, (key, value) in enumerate(info.items()):
			last = i == len(info) - 1
			for line in self.initialiser_field(key, value, last):
				yield "\t" + line

	def local_struct(self, label: str, struct_type: str, fields: dict, /) -> Iterable[str]:
		yield f"const {struct_type} {label} ="
		yield "{"
		for line in self.initialiser_fields(fields):
			yield line
		yield "};"

	@staticmethod
	def func(func_str: str, lesson_num: int) -> str:
		return "\n" + Template(func_str).substitute({"lesson_num": f"{lesson_num}"})

	def template_mapping(self, o: Options) -> dict[str, str]:
		def macros_from_condition(cond: bool, name: str) -> dict[str, str]:
			on = self.conditions.get(name, {})
			off = self.conditions.get(f"-{name}", {})
			return dict((macro, on.get(macro, "") if cond else off.get(macro, ""))
				for macro in on.keys() | off.keys())

		return {
			"copyright_text": f"(C) {o.copyright_year} a dinosaur",
			"copyright_license": "Zlib",
			"lesson_num": f"{o.lesson_num}",
			"lesson_title": self.sanitise_string(o.title),
			"lesson_definitions": self.global_field("projection", self._matrix_type) if o.projection else self._empty_globals_semicolon,
			"lesson_struct_depth": "" if o.depthfmt_suffix is None else f"\n{self._struct_depth}",
			"lesson_pass_depth": self._null if o.depthfmt_suffix is None else self._depth_info_ref,
			"lesson_func_resize": self.func(self.func_resize_projection, o.lesson_num) if o.projection else "",
			"lesson_func_key": self.func(self.func_keys, o.lesson_num) if o.key else "",
			"appconfig_depthfmt": "" if o.depthfmt_suffix is None else Template(self._appconfig_depthfmt).substitute({'depth_format': f'SDL_GPU_TEXTUREFORMAT_{o.depthfmt_suffix}'}),
			"appconfig_resize": f"Lesson{o.lesson_num}_Resize" if o.projection else "NULL",
			"appconfig_key": f",\n\t{'\t\n'.join(self.initialiser_field('key', f'Lesson{o.lesson_num}_Key', True))}" if o.key else "",
			**macros_from_condition(o.projection, "projection"),
			**macros_from_condition(o.key, "key"),
			**macros_from_condition(bool(o.depthfmt_suffix), "depth_format"),
		}


def main():
	o = Options()

	import sys
	basedir = Path(sys.argv[0]).resolve().parent
	template_dir = basedir.joinpath("templates")

	source_gen = SourceGenerator(o.lang, template_dir)
	dest_dir = Path(source_gen.source_dir)
	if source_gen.swift_subdir:
		dest_dir = dest_dir / f"Lesson{o.lesson_num:02d}"
	root = basedir.parent
	root.joinpath(dest_dir).mkdir(parents=True, exist_ok=True)

	template_filename = f"{o.template_name}.{o.lang}.txt"
	if source_gen.swift_subdir:
		output_filename = f"lesson{o.lesson_num}.{source_gen.file_extension}"
	else:
		output_filename = f"lesson{o.lesson_num:02d}.{source_gen.file_extension}"

	with template_dir.joinpath(template_filename).open("r") as src:
		t = Template(src.read())
	with root.joinpath(dest_dir, output_filename).open("w") as out:
		out.write(t.substitute(source_gen.template_mapping(o)))


if __name__ == "__main__":
	main()
