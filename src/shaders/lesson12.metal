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
	float3 normal   [[attribute(1)]];
	float2 texCoord [[attribute(2)]];
	float  tint     [[attribute(3)]];

	// Instance
	float4 model0 [[attribute(4)]];
	float4 model1 [[attribute(5)]];
	float4 model2 [[attribute(6)]];
	float4 model3 [[attribute(7)]];
	float4 color  [[attribute(8)]];
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

vertex Vertex2Fragment VertexMain(
	VertexInput in [[stage_in]],
	constant VertexUniform& u [[buffer(0)]])
{
	const auto model = metal::float4x4(in.model0, in.model1, in.model2, in.model3);
	const auto modelView = u.view * model;
	const auto normal = metal::normalize(modelView * float4(in.normal, 0.0)).xyz;

	constexpr auto lightDir = float3(0.0, 0.0, 1.0);
	const auto lambert = metal::max(0.0, metal::dot(normal, lightDir));

	Vertex2Fragment out;
	out.position = u.projection * modelView * float4(in.position, 1.0);
	out.texCoord = in.texCoord;
	// Apply lighting as if we were in GL_COLOR_MATERIAL defaults
	out.color = half4(in.color) * half4(half3(in.tint * (0.2 + lambert)), 1.0);
	return out;
}

fragment half4 FragmentMain(
	Vertex2Fragment in [[stage_in]],
	metal::texture2d<half, metal::access::sample> texture [[texture(0)]],
	metal::sampler sampler [[sampler(0)]])
{
	return in.color * texture.sample(sampler, in.texCoord);
}
