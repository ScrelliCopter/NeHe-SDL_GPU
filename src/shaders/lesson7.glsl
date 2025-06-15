/*
 * SPDX-FileCopyrightText: (C) 2025 a dinosaur
 * SPDX-License-Identifier: Zlib
 */

#version 450

#ifdef VERTEX

layout(location = 0) in vec3 i_position;
layout(location = 1) in vec2 i_texcoord;
layout(location = 2) in vec3 i_normal;

layout(location = 0) out vec2 v_texcoord;
layout(location = 1) out vec4 v_color;

layout(set = 1, binding = 0) uniform UBO
{
	mat4 u_modelView;
	mat4 u_projection;
};

layout(set = 1, binding = 1) uniform Light
{
	vec4 u_ambient;
	vec4 u_diffuse;
	vec4 u_position;
};

void main()
{
	const vec4 position = u_modelView * vec4(i_position, 1.0);
	const vec3 normal = normalize(u_modelView * vec4(i_normal, 0.0)).xyz;

	const vec3 lightVec = u_position.xyz - position.xyz;
	const float lightDest2 = dot(lightVec, lightVec);
	const vec3 dir = inversesqrt(lightDest2) * lightVec;
	const float lambert = max(0.0, dot(normal, dir));

	const vec3 ambient = 0.04 + 0.2 * u_ambient.rgb;
	const vec3 diffuse = 0.8 * u_diffuse.rgb;

	v_texcoord  = i_texcoord;
	v_color     = vec4(ambient + lambert * diffuse, 1.0);
	gl_Position = u_projection * position;
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
