/*
 * SPDX-FileCopyrightText: (C) 2025 a dinosaur
 * SPDX-License-Identifier: Zlib
 */

#include <metal_stdlib>
#include <simd/simd.h>

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

static constexpr constant float2 quadVertices[4] =
{
	{ -1.1f, -1.1f },  // Bottom left
	{  1.1f, -1.1f },  // Bottom right
	{ -1.1f,  1.1f },  // Top left
	{  1.1f,  1.1f }   // Top right
};

static constexpr constant float2 quadTexCoords[4] =
{
    { 0.0f, 0.0f },  // Bottom left
    { 1.0f, 0.0f },  // Bottom right
    { 0.0f, 1.0f },  // Top left
    { 1.0f, 1.0f }   // Top right
};

vertex Vertex2Fragment VertexMain(
	uint vertexID [[vertex_id]],
	constant VertexUniform& u [[buffer(0)]])
{
	Vertex2Fragment out;
	out.position = u.modelViewProj * float4(quadVertices[vertexID], 0.0, 1.0);
	out.texCoord = u.texOffset + quadTexCoords[vertexID] * u.texScale;
	return out;
}

fragment half4 FragmentMain(
	Vertex2Fragment in [[stage_in]],
	metal::texture2d<half, metal::access::sample> texture [[texture(0)]],
	metal::sampler sampler [[sampler(0)]])
{
	return texture.sample(sampler, in.texCoord);
}
