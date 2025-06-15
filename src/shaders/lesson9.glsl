/*
 * SPDX-FileCopyrightText: (C) 2025 a dinosaur
 * SPDX-License-Identifier: Zlib
 */

#version 450

#ifdef VERTEX

layout(location = 0) in vec3 i_position;
layout(location = 1) in vec2 i_texcoord;

layout(location = 0) out vec2 v_texcoord;
layout(location = 1) out vec4 v_color;

layout(set = 1, binding = 0) uniform UBO
{
	mat4 u_projection;
	vec4 u_color;
};

struct Instance
{
	mat4 model;
	vec4 color;
};

layout(std140, set = 0, binding = 0) readonly buffer InstanceData
{
	Instance instances[];
} instanceData;

void main()
{
	const Instance instance = instanceData.instances[gl_InstanceIndex];

	v_texcoord  = i_texcoord;
	v_color = instance.color;
	gl_Position = u_projection * instance.model * vec4(i_position, 1.0);
}

#else

layout(location = 0) in vec2 v_texcoord;
layout(location = 1) in vec4 v_color;

layout(location = 0) out vec4 o_color;

layout(set = 2, binding = 0) uniform sampler2D u_texture;

void main()
{
	o_color = v_color * texture(u_texture, v_texcoord);
}

#endif
