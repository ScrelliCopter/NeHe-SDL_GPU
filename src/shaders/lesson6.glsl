/*
 * SPDX-FileCopyrightText: (C) 2025 a dinosaur
 * SPDX-License-Identifier: Zlib
 */

#version 450

#ifdef VERTEX

layout(location = 0) in vec3 i_position;
layout(location = 1) in vec2 i_texcoord;

layout(location = 0) out vec2 v_texcoord;

layout(set = 1, binding = 0) uniform UBO
{
	mat4 u_viewproj;
};

void main()
{
	v_texcoord  = i_texcoord;
	gl_Position = u_viewproj * vec4(i_position, 1.0);
}

#else

layout(location = 0) in vec2 v_texcoord;

layout(location = 0) out vec4 o_color;

layout(set = 2, binding = 0) uniform sampler2D u_texture;

void main()
{
	o_color = texture(u_texture, v_texcoord);
}

#endif
