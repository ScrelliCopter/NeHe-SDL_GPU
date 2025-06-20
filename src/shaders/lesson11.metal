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
	metal::float4x4 viewProj;
	float waveOffset;
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
	constexpr float rescale = 44.0 / 45.0, tau = 6.283185308;
	const float z = metal::sin((in.texCoord.x * rescale + u.waveOffset) * tau);
	const float4 position = float4(in.position, z, 1.0);

	Vertex2Fragment out;
	out.position = u.viewProj * position;
	out.texCoord = in.texCoord;
	return out;
}

fragment half4 FragmentMain(
	Vertex2Fragment in [[stage_in]],
	metal::texture2d<half, metal::access::sample> texture [[texture(0)]],
	metal::sampler sampler [[sampler(0)]])
{
	return texture.sample(sampler, in.texCoord);
}
