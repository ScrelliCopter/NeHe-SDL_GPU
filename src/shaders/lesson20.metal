/*
 * SPDX-FileCopyrightText: (C) 2025 a dinosaur
 * SPDX-License-Identifier: Zlib
 */

#include <metal_stdlib>
#include <simd/simd.h>

struct VertexInput
{
	float2 position [[attribute(0)]];
	float2 texCoord [[attribute(1)]];
};

struct VertexUniform
{
	metal::float4x4 modelViewProj;
    float2 texOffset, texScale;
};

struct Vertex2Fragment
{
	float4 position [[position]];
	float2 texCoord;
};

vertex Vertex2Fragment VertexMain(
	VertexInput in [[stage_in]],
	constant VertexUniform& u [[buffer(0)]])
{
	Vertex2Fragment out;
	out.position = u.modelViewProj * float4(in.position, 0.0, 1.0);
	out.texCoord = u.texOffset + in.texCoord * u.texScale;
	return out;
}

fragment half4 FragmentMain(
	Vertex2Fragment in [[stage_in]],
	metal::texture2d<half, metal::access::sample> texture [[texture(0)]],
	metal::sampler sampler [[sampler(0)]])
{
	return texture.sample(sampler, in.texCoord);
}
