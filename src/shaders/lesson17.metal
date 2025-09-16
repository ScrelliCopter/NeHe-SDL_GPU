/*
 * SPDX-FileCopyrightText: (C) 2025 a dinosaur
 * SPDX-License-Identifier: Zlib
 */

#include <metal_stdlib>
#include <simd/simd.h>

struct CharacterInput
{
	float3 color [[attribute(0)]];
	float2 position [[attribute(1)]];
	int character [[attribute(2)]];
};

struct VertexUniform
{
	metal::float4x4 modelViewProj;
};

struct Vertex2Fragment
{
	float4 position [[position]];
	float2 texCoord;
	half4 color;
};

vertex Vertex2Fragment VertexMain(
	CharacterInput in [[stage_in]],
	constant VertexUniform& u [[buffer(0)]],
	uint vertexID [[vertex_id]])
{
	const auto offset = float2(vertexID >> 1, 1 - (vertexID & 0x1));
	const auto source = float2(in.character & 0xF, 15 - (in.character >> 4));

	Vertex2Fragment out;
	out.position = u.modelViewProj * float4(in.position + 16.0 * offset, 0.0, 1.0);
	out.texCoord = 0.0625 * (source + offset);
	out.color = half4(half3(in.color), 1.0);
	return out;
}

fragment float4 FragmentMain(
	Vertex2Fragment in [[stage_in]],
	metal::texture2d<half, metal::access::sample> texture [[texture(0)]],
	metal::sampler sampler [[sampler(0)]])
{
	return in.color * texture.sample(sampler, in.texCoord);
}
