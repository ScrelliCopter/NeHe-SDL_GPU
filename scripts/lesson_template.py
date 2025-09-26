#!/usr/bin/env python3
"""
SPDX-FileCopyrightText: (C) 2025 a dinosaur
SPDX-License-Identifier: Zlib
"""

import sys
from pathlib import Path
from datetime import datetime
from argparse import ArgumentParser
from typing import Iterable, List
from collections import namedtuple


head_copyright = [
	f"SPDX-FileCopyrightText: (C) {datetime.now().year} a dinosaur",
	"SPDX-License-Identifier: Zlib",
]

cube_vertices = {
	"Front Face": [[-1,-1,1,0,0,1,0,0], [1,-1,1,0,0,1,1,0], [1,1,1,0,0,1,1,1], [-1,1,1,0,0,1,0,1]],
	"Back Face": [[-1,-1,-1,0,0,-1,1,0], [-1,1,-1,0,0,-1,1,1], [1,1,-1,0,0,-1,0,1], [1,-1,-1,0,0,-1,0,0]],
	"Top Face": [[-1,1,-1,0,1,0,0,1], [-1,1,1,0,1,0,0,0], [1,1,1,0,1,0,1,0], [1,1,-1,0,1,0,1,1]],
	"Bottom Face": [[-1,-1,-1,0,-1,0,1,1], [1,-1,-1,0,-1,0,0,1], [1,-1,1,0,-1,0,0,0], [-1,-1,1,0,-1,0,1,0]],
	"Right face": [[1,-1,-1,1,0,0,1,0], [1,1,-1,1,0,0,1,1], [1,1,1,1,0,0,0,1], [1,-1,1,1,0,0,0,0]],
	"Left Face": [[-1,-1,-1,-1,0,0,0,0], [-1,-1,1,-1,0,0,1,0], [-1,1,1,-1,0,0,1,1], [-1,1,-1,-1,0,0,0,1]]
}

cube_indices = {
	"Front":  [ 0,  1,  2,   2,  3,  0],
	"Back":   [ 4,  5,  6,   6,  7,  4],
	"Top":    [ 8,  9, 10,  10, 11,  8],
	"Bottom": [12, 13, 14,  14, 15, 12],
	"Right":  [16, 17, 18,  18, 19, 16],
	"Left":   [20, 21, 22,  22, 23, 20],
}

depth_texfmt_suffixes = {
	16: "D16_UNORM", 24: "D24_UNORM", 32: "D32_FLOAT",
	248: "D24_UNORM_S8_UINT", 328: "D32_FLOAT_S8_UINT", 48: "D32_FLOAT_S8_UINT" }

VertexAttrib = namedtuple("VertexAttrib", ["location", "buffer_slot", "format", "offsetof"])
lesson6_vtx_attribs = [
	VertexAttrib(0, 0, "FLOAT3", "x"),
	VertexAttrib(1, 0, "FLOAT2", "u")
]
lesson7_vtx_attribs = lesson6_vtx_attribs + [VertexAttrib(2, 0, "FLOAT3", "nx")]

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
	def __init__(self, type: str, elements: List[dict]):
		self.type = type
		self.elements = elements

pipeline_info = {
	"vertex_shader": "vertexShader",
	"fragment_shader": "fragmentShader",
	"primitive_type": "SDL_GPU_PRIMITIVETYPE_TRIANGLELIST",
	"vertex_input_state": {
		"vertex_buffer_descriptions": Array("SDL_GPUVertexBufferDescription", [{
			"slot": 0,
			"pitch": SizeOf("Vertex"),
			"input_rate": "SDL_GPU_VERTEXINPUTRATE_VERTEX"
		}]),
		"num_vertex_buffers": 1,
		"vertex_attributes": "vertexAttribs",
		"num_vertex_attributes": "SDL_arraysize(vertexAttribs)",
	},
	"rasterizer_state": {
		"fill_mode": "SDL_GPU_FILLMODE_FILL",
		"cull_mode": "SDL_GPU_CULLMODE_NONE",
		"front_face": Value("SDL_GPU_FRONTFACE_COUNTER_CLOCKWISE", "Right-handed coordinates"),
		"enable_depth_clip": Value(True, "OpenGL-like clip behaviour")
	},
	"depth_stencil_state": {
		"compare_op": "SDL_GPU_COMPAREOP_LESS_OR_EQUAL",
		"enable_depth_test": True,
		"enable_depth_write": True
	},
	"target_info": {
		"color_target_descriptions": Array("SDL_GPUColorTargetDescription", [{
			"format": "SDL_GetGPUSwapchainTextureFormat(ctx->device, ctx->window)"
		}]),
		"num_color_targets": 1,
		"depth_stencil_format": "appConfig.createDepthFormat",
		"has_depth_stencil_target": True
	}
}


class Writer:
	pass

class Options:
	pass


def write_lesson(w: Writer, o: Options):
	w.wl(
		"/*",
		*(f" * {line}" for line in head_copyright),
		" */",
		"",
		'#include "nehe.h"',
		"",
		"")
	if o.lesson6 or o.lesson7:
		def vertices() -> Iterable[str]:
			for i, (k, v) in enumerate(cube_vertices.items()):
				yield f"\t// {k}"
				for j, l in enumerate(v):
					sep = ',' if (i < len(cube_vertices) - 1) or (j < len(v) - 1) else ''
					if o.lesson7:
						yield f"\t{{ {l[0]:>4.1f}f, {l[1]:>4.1f}f, {l[2]:>4.1f}f,  {l[3]:>4.1f}f, {l[4]:>4.1f}f, {l[5]:>4.1f}f,  {l[6]:.1f}f, {l[7]:.1f}f }}{sep}"
					else:
						yield f"\t{{ {l[0]:>4.1f}f, {l[1]:>4.1f}f, {l[2]:>4.1f}f, {l[6]:.1f}f, {l[7]:.1f}f }}{sep}"

		def indices() -> Iterable[str]:
			for i, (k, v) in enumerate(cube_indices.items()):
				last = i == len(cube_indices) - 1
				yield f"\t{v[0]:>2}, {v[1]:>2}, {v[2]:>2},  {v[3]:>2}, {v[4]:>2}, {v[5]:>2}{' ' if last else ','}  // {k}"

		w.wl(
			"typedef struct",
			"{",
			"	float x, y, z;")
		if o.lesson7:
			w.wl(
				"	float nx, ny, nz;")
		w.wl(
			"	float u, v;",
			"} Vertex;",
			"",
			"static const Vertex vertices[] =",
			"{",
			*vertices(),
			"};",
			"",
			"static const uint16_t indices[] =",
			"{",
			*indices(),
			"};",
			"",
			"")

	if o.lesson6 or o.lesson7:
		w.wl(
			f"static SDL_GPUGraphicsPipeline* pso{'Unlit = NULL, * psoLight' if o.lesson7 else ''} = NULL;",
			"static SDL_GPUBuffer* vtxBuffer = NULL;",
			"static SDL_GPUBuffer* idxBuffer = NULL;",
			f"static SDL_GPUSampler* sampler{'s[3] = { NULL, NULL, NULL }' if o.lesson7 else ' = NULL'};",
			"static SDL_GPUTexture* texture = NULL;",
			"")
	if o.lesson_projection:
		w.wl("static Mtx projection;\n")
	if o.lesson6:
		w.wl(
			"static float xRot = 0.0f, yRot = 0.0f, zRot = 0.0f;",
			"")
	if o.lesson7:
		w.wl(
			"static bool lighting = false;",
			"static struct Light { float ambient[4], diffuse[4], position[4]; } light =",
			"{",
			"	.ambient  = { 0.5f, 0.5f, 0.5f, 1.0f },",
			"	.diffuse  = { 1.0f, 1.0f, 1.0f, 1.0f },",
			"	.position = { 0.0f, 0.0f, 2.0f, 1.0f }",
			"};",
			"",
			"static int filter = 0;",
			"",
			"static float xRot = 0.0f, yRot = 0.0f;",
			"static float xSpeed = 0.0f, ySpeed = 0.0f;",
			"static float z = -5.0f;",
			"")
	w.wl("")
	if o.lesson_full:
		w.wl(
			f"static bool Lesson{o.lesson_num}_Init(NeHeContext* restrict ctx)",
			"{")
		if o.lesson6 or o.lesson7:
			if o.lesson6:
				w.wl(
					"	SDL_GPUShader* vertexShader, * fragmentShader;",
					'	if (!NeHe_LoadShaders(ctx, &vertexShader, &fragmentShader, "lesson6",')
			elif o.lesson7:
				w.wl(
					"	SDL_GPUShader* vertexShaderUnlit, * fragmentShaderUnlit;",
					"	SDL_GPUShader* vertexShaderLight, * fragmentShaderLight;",
					'	if (!NeHe_LoadShaders(ctx, &vertexShaderUnlit, &fragmentShaderUnlit, "lesson6",')
			w.wl(
				"		&(const NeHeShaderProgramCreateInfo){ .vertexUniforms = 1, .fragmentSamplers = 1 }))",
				"	{",
				"		return false;",
				"	}")
			if o.lesson7:
				w.wl(
					'	if (!NeHe_LoadShaders(ctx, &vertexShaderLight, &fragmentShaderLight, "lesson7",',
					"		&(const NeHeShaderProgramCreateInfo){ .vertexUniforms = 2, .fragmentSamplers = 1 }))",
					"	{",
					"		SDL_ReleaseGPUShader(ctx->device, fragmentShaderUnlit);",
					"		SDL_ReleaseGPUShader(ctx->device, vertexShaderUnlit);",
					"		return false;",
					"	}")
			w.wl(
				"",
				"	const SDL_GPUVertexAttribute vertexAttribs[] =",
				"	{")
			def write_attribs(attribs: List[VertexAttrib]) -> Iterable[str]:
				for i, attrib in enumerate(attribs):
					yield "\t\t{"
					yield f"\t\t\t.location = {attrib.location},"
					yield f"\t\t\t.buffer_slot = {attrib.buffer_slot},"
					yield f"\t\t\t.format = SDL_GPU_VERTEXELEMENTFORMAT_{attrib.format},"
					yield f"\t\t\t.offset = offsetof(Vertex, {attrib.offsetof})"
					yield "\t\t}," if i < len(attribs) - 1 else "\t\t}"
			w.wl(*write_attribs(lesson7_vtx_attribs if o.lesson7 else lesson6_vtx_attribs))
			w.wl(
				"	};")
			def write_pso(info) -> Iterable[str]:
				for i, (k, v) in enumerate(info.items()):
					last = i < len(info) - 1
					if isinstance(v, Value):
						value = v
					elif isinstance(v, SizeOf):
						value = Value(f"sizeof({v.struct})")
					elif isinstance(v, ArraySizeOf):
						value = Value(f"SDL_arraysize({v.array})")
					elif isinstance(v, OffsetOf):
						value = Value(f"offsetof({v.struct}, {v.field})")
					elif isinstance(v, Array):
						yield f".{k} = &(const {v.type})"
						yield "{"
						if len(v.elements) == 1:
							for line in write_pso(v.elements[0]):
								yield "\t" + line
						yield "}," if last else "}"
						continue
					elif isinstance(v, dict):
						yield f".{k} ="
						yield "{"
						for line in write_pso(v):
							yield "\t" + line
						yield "}," if last else "}"
						continue
					else:
						value = Value(v)
					strval = str(value.value)
					if isinstance(value.value, bool):
						strval = strval.lower()
					yield f".{k} = {strval}{',' if last else ''}{'  // ' + value.comment if value.comment else ''}"
			if o.lesson6:
				w.wl("\tpso = SDL_CreateGPUGraphicsPipeline(ctx->device, &(const SDL_GPUGraphicsPipelineCreateInfo)",
					"	{",
					 *("\t\t" + line for line in write_pso(pipeline_info)),
					"	});",
					"	SDL_ReleaseGPUShader(ctx->device, fragmentShader);",
					"	SDL_ReleaseGPUShader(ctx->device, vertexShader);",
					"	if (!pso)",
					"	{",
					'		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "SDL_CreateGPUGraphicsPipeline: %s", SDL_GetError());',
					"		return false;",
					"	}",
					"", )
			elif o.lesson7:
				common = {
					"primitive_type": "SDL_GPU_PRIMITIVETYPE_TRIANGLELIST",
					"vertex_input_state": "vertexInput",
					"rasterizer_state": "rasterizer",
					"depth_stencil_state": "depthStencil",
					"target_info": "targetInfo"
				}
				w.wl(
					"",
					"	const SDL_GPUVertexInputState vertexInput =",
					"	{",
					*("\t\t" + line for line in write_pso(pipeline_info["vertex_input_state"])),
					"	};",
					"",
					"	const SDL_GPURasterizerState rasterizer =",
					"	{",
					*("\t\t" + line for line in write_pso(pipeline_info["rasterizer_state"])),
					"	};",
					"",
					"	const SDL_GPUDepthStencilState depthStencil =",
					"	{",
					*("\t\t" + line for line in write_pso(pipeline_info["depth_stencil_state"])),
					"	};",
					"",
					"	const SDL_GPUGraphicsPipelineTargetInfo targetInfo =",
					"	{",
					*("\t\t" + line for line in write_pso(pipeline_info["target_info"])),
					"	};",
					"",
					"	psoUnlit = SDL_CreateGPUGraphicsPipeline(ctx->device, &(const SDL_GPUGraphicsPipelineCreateInfo)",
					"	{",
					*("\t\t" + line for line in write_pso({
						"vertex_shader": "vertexShaderUnlit",
						"fragment_shader": "fragmentShaderUnlit",
						**common
					})),
					"	});",
					"	SDL_ReleaseGPUShader(ctx->device, fragmentShaderUnlit);",
					"	SDL_ReleaseGPUShader(ctx->device, vertexShaderUnlit);",
					"	if (!psoUnlit)",
					"	{",
					'		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "SDL_CreateGPUGraphicsPipeline: %s", SDL_GetError());',
					"		SDL_ReleaseGPUShader(ctx->device, fragmentShaderLight);",
					"		SDL_ReleaseGPUShader(ctx->device, vertexShaderLight);",
					"		return false;",
					"	}",
					"",

					"	psoLight = SDL_CreateGPUGraphicsPipeline(ctx->device, &(const SDL_GPUGraphicsPipelineCreateInfo)",
					"	{",
					*("\t\t" + line for line in write_pso({
						"vertex_shader": "vertexShaderLight",
						"fragment_shader": "fragmentShaderLight",
						**common
					})),
					"	});",
					"	SDL_ReleaseGPUShader(ctx->device, fragmentShaderLight);",
					"	SDL_ReleaseGPUShader(ctx->device, vertexShaderLight);",
					"	if (!psoLight)",
					"	{",
					'		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "SDL_CreateGPUGraphicsPipeline: %s", SDL_GetError());',
					"		return false;",
					"	}",
					"")
			w.wl(
				f'	if ((texture = NeHe_LoadTexture(ctx, "Data/{"Crate" if o.lesson7 else "NeHe"}.bmp", true, {str(o.lesson7).lower()})) == NULL)',
				"	{",
				"		return false;",
				"	}",
				"")
			if o.lesson6:
				w.wl(
				"	sampler = SDL_CreateGPUSampler(ctx->device, &(const SDL_GPUSamplerCreateInfo)",
				"	{",
				"		.min_filter = SDL_GPU_FILTER_LINEAR,",
				"		.mag_filter = SDL_GPU_FILTER_LINEAR",
				"	});",
				"	if (!sampler)")
			elif o.lesson7:
				w.wl(
					"	samplers[0] = SDL_CreateGPUSampler(ctx->device, &(const SDL_GPUSamplerCreateInfo)",
					"	{",
					"		.min_filter = SDL_GPU_FILTER_NEAREST,",
					"		.mag_filter = SDL_GPU_FILTER_NEAREST",
					"	});",
					"	samplers[1] = SDL_CreateGPUSampler(ctx->device, &(const SDL_GPUSamplerCreateInfo)",
					"	{",
					"		.min_filter = SDL_GPU_FILTER_LINEAR,",
					"		.mag_filter = SDL_GPU_FILTER_LINEAR",
					"	});",
					"	samplers[2] = SDL_CreateGPUSampler(ctx->device, &(const SDL_GPUSamplerCreateInfo)",
					"	{",
					"		.min_filter = SDL_GPU_FILTER_LINEAR,",
					"		.mag_filter = SDL_GPU_FILTER_LINEAR,",
					"		.mipmap_mode = SDL_GPU_SAMPLERMIPMAPMODE_NEAREST,",
					"		.max_lod = FLT_MAX",
					"	});",
					"	if (!samplers[0] || !samplers[1] || !samplers[2])")
			w.wl(
				"	{",
				'		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "SDL_CreateGPUSampler: %s", SDL_GetError());',
				"		return false;",
				"	}",
				"",
				"	if (!NeHe_CreateVertexIndexBuffer(ctx, &vtxBuffer, &idxBuffer,",
				"		vertices, sizeof(vertices),",
				"		indices, sizeof(indices)))",
				"	{",
				"		return false;",
				"	}")
		else:
			w.wl("	")
		w.wl(
			"",
			"	return true;",
			"}",
			"",
			f"static void Lesson{o.lesson_num}_Quit(NeHeContext* restrict ctx)",
			"{")
		if o.lesson6 or o.lesson7:
			w.wl(
				"	SDL_ReleaseGPUBuffer(ctx->device, idxBuffer);",
				"	SDL_ReleaseGPUBuffer(ctx->device, vtxBuffer);")
			if o.lesson6:
				w.wl(
					"	SDL_ReleaseGPUSampler(ctx->device, sampler);")
			else:
				w.wl(
					"	for (int i = SDL_arraysize(samplers) - 1; i > 0; --i)",
					"	{",
					"		SDL_ReleaseGPUSampler(ctx->device, samplers[i]);",
					"	}")
			w.wl(
				"	SDL_ReleaseGPUTexture(ctx->device, texture);")
			if o.lesson6:
				w.wl(
					"	SDL_ReleaseGPUGraphicsPipeline(ctx->device, pso);")
			else:
				w.wl(
					"	SDL_ReleaseGPUGraphicsPipeline(ctx->device, psoLight);",
					"	SDL_ReleaseGPUGraphicsPipeline(ctx->device, psoUnlit);")
		else:
			w.wl("	")
		w.wl(
			"}",
			"")
	if o.lesson_projection:
		w.wl(
			f"static void Lesson{o.lesson_num}_Resize(NeHeContext* restrict ctx, int width, int height)",
			"{",
			"	(void)ctx;",
			"",
			"	// Avoid division by zero by clamping height",
			"	height = SDL_max(height, 1);",
			"	// Recalculate projection matrix",
			"	projection = Mtx_Perspective(45.0f, (float)width / (float)height, 0.1f, 100.0f);",
			"}",
			"")
	w.wl(
		f"static void Lesson{o.lesson_num}_Draw(NeHeContext* restrict ctx, SDL_GPUCommandBuffer* restrict cmd,",
		"	SDL_GPUTexture* restrict swapchain, unsigned swapchainW, unsigned swapchainH)",
		"{",
		"	(void)swapchainW; (void)swapchainH;",
		"",
		"	const SDL_GPUColorTargetInfo colorInfo =",
		"	{",
		"		.texture = swapchain,",
		"		.clear_color = { 0.0f, 0.0f, 0.0f, 0.5f },",
		"		.load_op = SDL_GPU_LOADOP_CLEAR,",
		"		.store_op = SDL_GPU_STOREOP_STORE",
		"	};",
		"")
	if o.lesson_depthfmt is not None:
		w.wl(
			"	const SDL_GPUDepthStencilTargetInfo depthInfo =",
			"	{",
			"		.texture = ctx->depthTexture,",
			"		.clear_depth = 1.0f,  // Ensure depth buffer clears to furthest value",
			"		.load_op = SDL_GPU_LOADOP_CLEAR,",
			"		.store_op = SDL_GPU_STOREOP_DONT_CARE,",
			"		.stencil_load_op = SDL_GPU_LOADOP_DONT_CARE,",
			"		.stencil_store_op = SDL_GPU_STOREOP_DONT_CARE,",
			"		.cycle = true",
			"	};",
			"")
	if o.lesson6 or o.lesson7:
		w.wl("	// Begin pass & bind pipeline state")
	w.wl(f"\tSDL_GPURenderPass* pass = SDL_BeginGPURenderPass(cmd, &colorInfo, 1, {'NULL' if o.lesson_depthfmt is None else '&depthInfo'});")
	if o.lesson6 or o.lesson7:
		w.wl(
			f"\tSDL_BindGPUGraphicsPipeline(pass, {'lighting ? psoLight : psoUnlit' if o.lesson7 else 'pso'});",
			"")
		w.wl(
			"	// Bind texture",
			"	SDL_BindGPUFragmentSamplers(pass, 0, &(const SDL_GPUTextureSamplerBinding)",
			"	{",
			"		.texture = texture,",
			f"		.sampler = {'samplers[filter]' if o.lesson7 else 'sampler'}",
			"	}, 1);",
			"",
			"	// Bind vertex & index buffers",
			"	SDL_BindGPUVertexBuffers(pass, 0, &(const SDL_GPUBufferBinding)",
			"	{",
			"		.buffer = vtxBuffer,",
			"		.offset = 0",
			"	}, 1);",
			"	SDL_BindGPUIndexBuffer(pass, &(const SDL_GPUBufferBinding)",
			"	{",
			"		.buffer = idxBuffer,",
			"		.offset = 0",
			"	}, SDL_GPU_INDEXELEMENTSIZE_16BIT);",
			"",
			f"	// {"Setup the cube's model matrix" if o.lesson7 else "Move cube 5 units into the screen and apply some rotations"}",
			f"	Mtx model = Mtx_Translation(0.0f, 0.0f, {'z' if o.lesson7 else '-5.0f'});",
			"	Mtx_Rotate(&model, xRot, 1.0f, 0.0f, 0.0f);",
			"	Mtx_Rotate(&model, yRot, 0.0f, 1.0f, 0.0f);")
		if o.lesson6:
			w.wl("\tMtx_Rotate(&model, zRot, 0.0f, 0.0f, 1.0f);")
		w.wl(
			"",
			"	// Push shader uniforms")
		if o.lesson6:
			w.wl(
				"	Mtx modelViewProj = Mtx_Multiply(&projection, &model);",
				"	SDL_PushGPUVertexUniformData(cmd, 0, &modelViewProj, sizeof(Mtx));")
		elif o.lesson7:
			w.wl(
				"	if (lighting)",
				"	{",
				"		struct { Mtx model, projection; } u = { model, projection };",
				"		SDL_PushGPUVertexUniformData(cmd, 0, &u, sizeof(u));",
				"		SDL_PushGPUVertexUniformData(cmd, 1, &light, sizeof(light));",
				"	}",
				"	else",
				"	{",
				"		Mtx modelViewProj = Mtx_Multiply(&projection, &model);",
				"		SDL_PushGPUVertexUniformData(cmd, 0, &modelViewProj, sizeof(Mtx));",
				"	}")
		w.wl(
			"",
			"	// Draw textured cube",
			"	SDL_DrawGPUIndexedPrimitives(pass, SDL_arraysize(indices), 1, 0, 0, 0);")
	else:
		w.wl(
			"",
			"	")
	w.wl(
		"",
		"	SDL_EndGPURenderPass(pass);")
	if o.lesson6:
		w.wl(
			"",
			"	xRot += 0.3f;",
			"	yRot += 0.2f;",
			"	zRot += 0.4f;")
	elif o.lesson7:
		w.wl(
			"",
			"	const bool* keys = SDL_GetKeyboardState(NULL);",
			"",
			"	if (keys[SDL_SCANCODE_PAGEUP])   { z -= 0.02f; }",
			"	if (keys[SDL_SCANCODE_PAGEDOWN]) { z += 0.02f; }",
			"	if (keys[SDL_SCANCODE_UP])    { xSpeed -= 0.01f; }",
			"	if (keys[SDL_SCANCODE_DOWN])  { xSpeed += 0.01f; }",
			"	if (keys[SDL_SCANCODE_RIGHT]) { ySpeed += 0.01f; }",
			"	if (keys[SDL_SCANCODE_LEFT])  { ySpeed -= 0.01f; }",
			"",
			"	xRot += xSpeed;",
			"	yRot += ySpeed;")
	w.wl(
		"}",
		"")
	if o.lesson_key:
		w.wl(
			f"static void Lesson{o.lesson_num}_Key(NeHeContext* ctx, SDL_Keycode key, bool down, bool repeat)",
			"{",
			"	(void)ctx;",
			"",
			"	if (down && !repeat)",
			"	{",
			"		switch (key)",
			"		{")
		if o.lesson7:
			w.wl(
				"		case SDLK_L:",
				"			lighting = !lighting;",
				"			break;",
				"		case SDLK_F:",
				"			filter = (filter + 1) % (int)SDL_arraysize(samplers);",
				"			break;")
		w.wl(
			"		default:",
			"			break;",
			"		}",
			"	}",
			"}",
			"")
	w.wl("",
		f"const struct AppConfig appConfig =",
		"{",
		f'	.title = "{o.lesson_title.encode("unicode-escape").decode("utf-8")}",',
		"	.width = 640, .height = 480,")
	if o.lesson_depthfmt is not None:
		w.wl(f"\t.createDepthFormat = {o.lesson_depthfmt},")
	w.wl(
		f"	.init = {f'Lesson{o.lesson_num}_Init' if o.lesson_full else 'NULL'},",
		f"	.quit = {f'Lesson{o.lesson_num}_Quit' if o.lesson_full else 'NULL'},",
		f"	.resize = {f'Lesson{o.lesson_num}_Resize' if o.lesson_projection else 'NULL'},",
		f"	.draw = Lesson{o.lesson_num}_Draw{',' if o.lesson_key else ''}")
	if o.lesson_key:
		w.wl(f"\t.key = Lesson{o.lesson_num}_Key")
	w.wl("};")


class Options:
	def __init__(self):
		p = ArgumentParser()
		p.add_argument("num", type=int)
		p.add_argument("--depth", type=int)
		p.add_argument("--key", action="store_true")
		p.add_argument("--projection", action="store_true")
		p.add_argument("--title", default="")
		g = p.add_mutually_exclusive_group()
		g.add_argument("--minimal", dest="full", action="store_false")
		g.add_argument("--lesson6", action="store_true")
		g.add_argument("--lesson7", action="store_true")
		a = p.parse_args()

		self.lesson_num = a.num
		depth = a.depth or (16 if a.lesson6 or a.lesson7 else None)
		self.lesson_depthfmt = None if depth is None else f"SDL_GPU_TEXTUREFORMAT_{depth_texfmt_suffixes[depth]}"
		self.lesson_key = a.key or a.lesson7
		self.lesson_projection = a.projection or a.lesson6 or a.lesson7
		self.lesson_title = a.title
		self.lesson_full = a.full
		self.lesson6 = a.lesson6
		self.lesson7 = a.lesson7


class Writer:
	def __init__(self, path: Path, mode = "w"):
		self._hnd = path.open(mode)

	def __enter__(self):
		return self

	def __exit__(self, exc_type, exc_val, exc_tb):
		self._hnd.close()

	def wl(self, *lines: str):
		print(*lines, sep="\n", end="\n", file=self._hnd)
		#self._hnd.writelines([line + "\n" for line in lines])


def main():
	o = Options()
	c_dest_dir = Path("src/c")

	root = Path(sys.argv[0]).resolve().parent.parent
	root.joinpath(c_dest_dir).mkdir(parents=True, exist_ok=True)

	with Writer(c_dest_dir / f"lesson{o.lesson_num:02d}.c") as out:
		write_lesson(out, o)


if __name__ == "__main__":
	main()
