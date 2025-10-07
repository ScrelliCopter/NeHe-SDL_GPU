/*
 * SPDX-FileCopyrightText: (C) 2025 a dinosaur
 * SPDX-License-Identifier: Zlib
 */

#include <metal_stdlib>
#include <simd/simd.h>

struct InstanceInput
{
	float4 position [[attribute(0)]];
	float4 color [[attribute(1)]];
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

static constexpr constant float2 quadVertices[4] =
{
	float2( 0.5f,  0.5f),  // Top right
	float2(-0.5f,  0.5f),  // Top left
	float2( 0.5f, -0.5f),  // Bottom right
	float2(-0.5f, -0.5f)   // Bottom left
};

static constexpr constant float2 quadTexCoords[4] =
{
	float2(1.0f, 1.0f),  // Top right
	float2(0.0f, 1.0f),  // Top left
	float2(1.0f, 0.0f),  // Bottom right
	float2(0.0f, 0.0f)   // Bottom left
};

vertex Vertex2Fragment VertexMain(
	InstanceInput in [[stage_in]],
	constant VertexUniform& u [[buffer(0)]],
	uint vertexID [[vertex_id]])
{
	Vertex2Fragment out;
	out.position = u.modelViewProj * (float4(quadVertices[vertexID], 0.0, 0.0) + in.position);
	out.texCoord = quadTexCoords[vertexID];
	out.color = half4(in.color);
	return out;
}

fragment half4 FragmentMain(
	Vertex2Fragment in [[stage_in]],
	metal::texture2d<half, metal::access::sample> texture [[texture(0)]],
	metal::sampler sampler [[sampler(0)]])
{
	return in.color * texture.sample(sampler, in.texCoord);
}
