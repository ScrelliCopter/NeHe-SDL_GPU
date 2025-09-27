#!/usr/bin/env python3
"""
SPDX-FileCopyrightText: (C) 2025 a dinosaur
SPDX-License-Identifier: Zlib
"""

from pathlib import Path
from typing import Iterable, List


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


class SourceGenC:
	@staticmethod
	def file_extension() -> str:
		return "c"

	@staticmethod
	def sanitise_string(s: str, /) -> str:
		return s.encode("unicode-escape").decode("utf-8")

	@staticmethod
	def global_field(label: str, field_type: str, /) -> str:
		return f"static {field_type} {label};"

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
	def func_resize_projection(lesson_num: str, /) -> str:
		return f"""
static void Lesson{lesson_num}_Resize(NeHeContext* restrict ctx, int width, int height)
{{
	(void)ctx;

	// Avoid division by zero by clamping height
	height = SDL_max(height, 1);
	// Recalculate projection matrix
	projection = Mtx_Perspective(45.0f, (float)width / (float)height, 0.1f, 100.0f);
}}
"""

	@staticmethod
	def func_keys(lesson_num: str, /) -> str:
		return f"""
static void Lesson{lesson_num}_Key(NeHeContext* ctx, SDL_Keycode key, bool down, bool repeat)
{{
	(void)ctx;

	if (down && !repeat)
	{{
		switch (key)
		{{
		default:
			break;
		}}
	}}
}}
"""


class Options:
	depth_texfmt_suffixes = {
		16: "D16_UNORM", 24: "D24_UNORM", 32: "D32_FLOAT",
		248: "D24_UNORM_S8_UINT", 328: "D32_FLOAT_S8_UINT", 48: "D32_FLOAT_S8_UINT"
	}

	depth_info = {
		"texture": "ctx->depthTexture",
		"clear_depth": Value(1.0, comment="Ensure depth buffer clears to furthest value"),
		"load_op": "SDL_GPU_LOADOP_CLEAR",
		"store_op": "SDL_GPU_STOREOP_DONT_CARE",
		"stencil_load_op": "SDL_GPU_LOADOP_DONT_CARE",
		"stencil_store_op": "SDL_GPU_STOREOP_DONT_CARE",
		"cycle": True
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

	def template_mapping(self, gen: SourceGenC) -> dict[str, str]:
		return {
			"copyright_text": f"(C) {self.copyright_year} a dinosaur",
			"copyright_license": "Zlib",
			"lesson_num": f"{self.lesson_num}",
			"lesson_title": gen.sanitise_string(self.title),
			"lesson_definitions": f"\n{gen.global_field("projection", "Mtx")}\n\n" if self.projection else "",
			"lesson_struct_depth": "" if self.depthfmt_suffix is None else f"\n\t{'\n\t'.join(gen.local_struct('depthInfo', 'SDL_GPUDepthStencilTargetInfo', self.depth_info))}\n",
			"lesson_pass_depth": "NULL" if self.depthfmt_suffix is None else "&depthInfo",
			"lesson_func_resize": gen.func_resize_projection(self.lesson_num) if self.projection else "",
			"lesson_func_key": gen.func_keys(self.lesson_num) if self.key else "",
			"appconfig_depthfmt": "" if self.depthfmt_suffix is None else f"\n\t{'\t\n'.join(gen.initialiser_field('createDepthFormat', f'SDL_GPU_TEXTUREFORMAT_{self.depthfmt_suffix}'))}",
			"appconfig_resize": f"Lesson{self.lesson_num}_Resize" if self.projection else "NULL",
			"appconfig_key": f",\n\t{'\t\n'.join(gen.initialiser_field('key', f'Lesson{self.lesson_num}_Key', True))}" if self.key else ""
		}


def main():
	o = Options()
	c_dest_dir = Path("src/c")

	import sys
	basedir = Path(sys.argv[0]).resolve().parent
	root = basedir.parent
	root.joinpath(c_dest_dir).mkdir(parents=True, exist_ok=True)

	source_gen = SourceGenC()

	template_filename = f"{o.template_name}.{source_gen.file_extension()}.txt"
	output_filename = f"lesson{o.lesson_num:02d}.{source_gen.file_extension()}"

	from string import Template
	with basedir.joinpath("templates", template_filename).open("r") as src:
		t = Template(src.read())
	with root.joinpath(c_dest_dir, output_filename).open("w") as out:
		out.write(t.substitute(o.template_mapping(source_gen)))


if __name__ == "__main__":
	main()
