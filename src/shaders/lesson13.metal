/*
 * SPDX-FileCopyrightText: (C) 2025 a dinosaur
 * SPDX-License-Identifier: Zlib
 */

#include <metal_stdlib>
#include <simd/simd.h>

struct CharacterInput
{
	float4 src [[attribute(0)]], dst [[attribute(1)]];
};

struct VertexUniform
{
	metal::float4x4 viewProj;
	float4 color;
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
	const auto offset = float2(vertexID >> 1, vertexID & 0x1);

	Vertex2Fragment out;
	out.position = u.viewProj * float4(in.dst.xy + in.dst.zw * offset, 0.0, 1.0);
	out.texCoord = in.src.xy + in.src.zw * offset;
	out.color = half4(u.color);
	return out;
}

fragment half4 FragmentMain(
	Vertex2Fragment in [[stage_in]],
	metal::texture2d<half, metal::access::sample> texture [[texture(0)]],
	metal::sampler sampler [[sampler(0)]])
{
	return in.color * texture.sample(sampler, in.texCoord).a;
}
