/*
 * SPDX-FileCopyrightText: (C) 2025 a dinosaur
 * SPDX-License-Identifier: Zlib
 */

#include <metal_stdlib>
#include <simd/simd.h>

struct VertexInput
{
	// Vertex
	float3 position [[attribute(0)]];
	float2 texcoord [[attribute(1)]];

	// Instance
	float4 model0 [[attribute(2)]];
	float4 model1 [[attribute(3)]];
	float4 model2 [[attribute(4)]];
	float4 model3 [[attribute(5)]];
	float4 color  [[attribute(6)]];
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
	constant VertexUniform& u [[buffer(0)]])
{
	const auto model = metal::float4x4(in.model0, in.model1, in.model2, in.model3);

	Vertex2Fragment out;
	out.position = u.projection * model * float4(in.position, 1.0);
	out.texcoord = in.texcoord;
	out.color = half4(in.color);
	return out;
}

fragment half4 FragmentMain(
	Vertex2Fragment in [[stage_in]],
	metal::texture2d<half, metal::access::sample> texture [[texture(0)]],
	metal::sampler sampler [[sampler(0)]])
{
	return in.color * texture.sample(sampler, in.texcoord);
}
