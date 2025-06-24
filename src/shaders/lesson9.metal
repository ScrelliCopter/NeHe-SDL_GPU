/*
 * SPDX-FileCopyrightText: (C) 2025 a dinosaur
 * SPDX-License-Identifier: Zlib
 */

#include <metal_stdlib>
#include <simd/simd.h>

struct InstanceInput
{
	float3 position [[attribute(0)]];
	float3 color [[attribute(1)]];
	float2 angle [[attribute(2)]];
};

struct VertexUniform
{
	metal::float4x4 view, projection;
};

struct Vertex2Fragment
{
	float4 position [[position]];
	float2 texCoord;
	half4 color;
};


static constexpr constant float2 quadTexCoords[4] =
{
	{ 0.0f, 0.0f },
	{ 1.0f, 0.0f },
	{ 1.0f, 1.0f },
	{ 0.0f, 1.0f }
};

static constexpr constant uint16_t quadIndices[6] =
{
	0,  1,  2,
	2,  3,  0
};

vertex Vertex2Fragment VertexMain(
	InstanceInput in [[stage_in]],
	constant VertexUniform& u [[buffer(0)]],
	uint vertexID [[vertex_id]])
{
	const float2 quadVertices[4] =
	{
		{ -in.angle.x + in.angle.y, -in.angle.x - in.angle.y },
		{  in.angle.x + in.angle.y, -in.angle.x + in.angle.y },
		{  in.angle.x - in.angle.y,  in.angle.x + in.angle.y },
		{ -in.angle.x - in.angle.y,  in.angle.x - in.angle.y }
	};

	const auto quadIndex = quadIndices[vertexID];

	auto position = u.view * float4(in.position, 1.0);
	position.xy += quadVertices[quadIndex];

	Vertex2Fragment out;
	out.position = u.projection * position;
	out.texCoord = quadTexCoords[quadIndex];
	out.color = half4(half3(in.color), 1.0);
	return out;
}

fragment half4 FragmentMain(
	Vertex2Fragment in [[stage_in]],
	metal::texture2d<half, metal::access::sample> texture [[texture(0)]],
	metal::sampler sampler [[sampler(0)]])
{
	return in.color * texture.sample(sampler, in.texCoord);
}
