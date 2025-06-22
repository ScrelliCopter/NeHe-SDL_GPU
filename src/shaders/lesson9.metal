/*
 * SPDX-FileCopyrightText: (C) 2025 a dinosaur
 * SPDX-License-Identifier: Zlib
 */

#include <metal_stdlib>
#include <simd/simd.h>

struct Vertex
{
	float4 position;
	float2 texCoord;
};

struct InstanceInput
{
	float4 model0 [[attribute(0)]];
	float4 model1 [[attribute(1)]];
	float4 model2 [[attribute(2)]];
	float4 model3 [[attribute(3)]];
	float4 color  [[attribute(4)]];
};

struct VertexUniform
{
	metal::float4x4 projection;
};

struct Vertex2Fragment
{
	float4 position [[position]];
	float2 texCoord;
	half4 color;
};


static constexpr constant Vertex quadVertices[4] =
{
	{ .position = { -1.0f, -1.0f, 0.0f, 1.0f }, .texCoord = { 0.0f, 0.0f } },
	{ .position = {  1.0f, -1.0f, 0.0f, 1.0f }, .texCoord = { 1.0f, 0.0f } },
	{ .position = {  1.0f,  1.0f, 0.0f, 1.0f }, .texCoord = { 1.0f, 1.0f } },
	{ .position = { -1.0f,  1.0f, 0.0f, 1.0f }, .texCoord = { 0.0f, 1.0f } }
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
	const auto model = metal::float4x4(in.model0, in.model1, in.model2, in.model3);
	const auto quadIndex = quadIndices[vertexID];
	const auto& quadVertex = quadVertices[quadIndex];

	Vertex2Fragment out;
	out.position = u.projection * model * quadVertex.position;
	out.texCoord = quadVertex.texCoord;
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
