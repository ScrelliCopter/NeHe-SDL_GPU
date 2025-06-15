/*
 * SPDX-FileCopyrightText: (C) 2025 a dinosaur
 * SPDX-License-Identifier: Zlib
 */

#version 450

#ifdef VERTEX

layout(location = 0) in vec3 i_position;
layout(location = 1) in vec4 i_color;

layout(location = 0) out vec4 v_color;

layout(set = 1, binding = 0) uniform UBO
{
	mat4 u_viewproj;
};

void main()
{
	v_color = i_color;
	gl_Position = u_viewproj * vec4(i_position, 1.0);
}

#else

layout(location = 0) in vec4 v_color;

layout(location = 0) out vec4 o_color;

void main()
{
	o_color = v_color;
}

#endif
