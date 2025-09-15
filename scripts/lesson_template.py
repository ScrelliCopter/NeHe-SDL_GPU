#!/usr/bin/env python3
"""
SPDX-FileCopyrightText: (C) 2025 a dinosaur
SPDX-License-Identifier: Zlib
"""

import sys
from pathlib import Path
from datetime import datetime
from argparse import ArgumentParser


def main():
	p = ArgumentParser()
	p.add_argument("num", type=int)
	p.add_argument("--depth", type=int)
	p.add_argument("--key", action="store_true")
	p.add_argument("--projection", action="store_true")
	p.add_argument("--title", default="")
	p.add_argument("--minimal", dest="full", action="store_false")
	a = p.parse_args()

	lesson_num = a.num
	lesson_depth = a.depth
	lesson_key = a.key
	lesson_projection = a.projection
	lesson_title = a.title
	lesson_min = a.full

	c_dest_dir = Path("src/c")

	root = Path(sys.argv[0]).resolve().parent.parent
	root.joinpath(c_dest_dir).mkdir(parents=True, exist_ok=True)

	with c_dest_dir.joinpath(f"lesson{lesson_num:02d}.c").open("w") as out:
		out.write(f"""/*
 * SPDX-FileCopyrightText: (C) {datetime.now().year} a dinosaur
 * SPDX-License-Identifier: Zlib
 */

#include "nehe.h"


""")
		if lesson_projection:
			out.write(f"""static Mtx projection;


""")
		if lesson_min:
			out.write(f"""static bool Lesson{lesson_num}_Init(NeHeContext* restrict ctx)
{{
	return true;
}}

static void Lesson{lesson_num}_Quit(NeHeContext* restrict ctx)
{{
}}

""")
		if lesson_projection:
			out.write(f"""static void Lesson{lesson_num}_Resize(NeHeContext* restrict ctx, int width, int height)
{{
	(void)ctx;

	// Avoid division by zero by clamping height
	height = SDL_max(height, 1);
	// Recalculate projection matrix
	projection = Mtx_Perspective(45.0f, (float)width / (float)height, 0.1f, 100.0f);
}}

""")
		out.write(f"""static void Lesson{lesson_num}_Draw(NeHeContext* restrict ctx, SDL_GPUCommandBuffer* restrict cmd,
	SDL_GPUTexture* restrict swapchain, unsigned swapchainW, unsigned swapchainH)
{{
	(void)swapchainW; (void)swapchainH;

	const SDL_GPUColorTargetInfo colorInfo =
	{{
		.texture = swapchain,
		.clear_color = {{ 0.0f, 0.0f, 0.0f, 0.5f }},
		.load_op = SDL_GPU_LOADOP_CLEAR,
		.store_op = SDL_GPU_STOREOP_STORE
	}};

	SDL_GPURenderPass* pass = SDL_BeginGPURenderPass(cmd, &colorInfo, 1, NULL);

\t

	SDL_EndGPURenderPass(pass);
}}

""")
		if lesson_key:
			out.write(f"""static void Lesson{lesson_num}_Key(NeHeContext* ctx, SDL_Keycode key, bool down, bool repeat)
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

""")
		out.write(f"""const struct AppConfig appConfig =
{{
	.title = "{lesson_title.encode('unicode-escape').decode('utf-8')}",
	.width = 640, .height = 480,
""")
		if lesson_depth is not None:
			texfmt_depths = {
				16: "D16_UNORM", 24: "D24_UNORM", 32: "D32_FLOAT",
				248: "D24_UNORM_S8_UINT", 328: "D32_FLOAT_S8_UINT", 48: "D32_FLOAT_S8_UINT" }
			out.write(f"\t.createDepthFormat = SDL_GPU_TEXTUREFORMAT_{texfmt_depths[lesson_depth]},\n")
		out.write(f"""\t.init = {f'Lesson{lesson_num}_Init' if lesson_min else 'NULL'},
	.quit = {f'Lesson{lesson_num}_Quit' if lesson_min else 'NULL'},
	.resize = {f'Lesson{lesson_num}_Resize' if lesson_projection else 'NULL'},
	.draw = Lesson{lesson_num}_Draw{',' if lesson_key else ''}
""")
		if lesson_key:
			out.write(f"\t.key = Lesson{lesson_num}_Key\n")
		out.write("};\n")


if __name__ == "__main__":
	main()
