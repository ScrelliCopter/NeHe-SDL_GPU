#!/usr/bin/env python3
"""
SPDX-FileCopyrightText: (C) 2025 a dinosaur
SPDX-License-Identifier: Zlib
"""

import tomllib
from pathlib import Path
from string import Template


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


class SourceGenerator:
	def __init__(self, lang_name: str, template_dir: Path, /):
		with template_dir.joinpath(f"{lang_name}.toml").open("rb") as f:
			lang_toml = tomllib.load(f)

		self.source_dir = lang_toml["source_dir"]
		self.file_extension = lang_toml["file_extension"]
		self.swift_subdir = lang_toml.get("swift_subdir", False)
		self.conditions = lang_toml["condition"]

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

	def template_mapping(self, o: Options, /) -> dict[str, str]:
		builtins = {
			"copyright_text": f"(C) {o.copyright_year} a dinosaur",
			"copyright_license": "Zlib",
			"lesson_num": f"{o.lesson_num}",
			"lesson_title": self.sanitise_string(o.title),
			"depth_format": f"SDL_GPU_TEXTUREFORMAT_{o.depthfmt_suffix}" if o.depthfmt_suffix else "",
		}

		def macros_from_condition(cond: bool, name: str, /) -> dict[str, str]:
			on = self.conditions.get(name, {})
			off = self.conditions.get(f"-{name}", {})
			return dict((macro, Template(on.get(macro, "") if cond else off.get(macro, ""))
					.substitute(builtins))
				for macro in on.keys() | off.keys())

		return {
			**builtins,
			**macros_from_condition(o.projection, "projection"),
			**macros_from_condition(o.key, "key"),
			**macros_from_condition(bool(o.depthfmt_suffix), "depth"),
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
