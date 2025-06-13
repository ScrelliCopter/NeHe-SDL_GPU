/*
 * SPDX-FileCopyrightText: (C) 2025 a dinosaur
 * SPDX-License-Identifier: Zlib
 */

#include <metal_stdlib>
#include <simd/simd.h>

struct VertexInput
{
	float3 position [[attribute(0)]];
	float2 texcoord [[attribute(1)]];
};

struct VertexInstance
{
	metal::float4x4 model;
	float4 color;
};

struct VertexUniform
{
	metal::float4x4 projection;
};

struct Vertex2Fragment
{
	float4 position [[position]];
	float2 texcoord;
	half4 color;
};

vertex Vertex2Fragment VertexMain(
	VertexInput in [[stage_in]],
	const device VertexInstance* instance [[buffer(1)]],
	constant VertexUniform& u [[buffer(0)]],
	const uint instanceIdx [[instance_id]])
{
	Vertex2Fragment out;
	out.position = u.projection * instance[instanceIdx].model * float4(in.position, 1.0);
	out.texcoord = in.texcoord;
	out.color = half4(instance[instanceIdx].color);
	return out;
}

fragment half4 FragmentMain(
	Vertex2Fragment in [[stage_in]],
	metal::texture2d<half, metal::access::sample> texture [[texture(0)]],
	metal::sampler sampler [[sampler(0)]])
{
	return in.color * texture.sample(sampler, in.texcoord);
}
