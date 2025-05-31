/*
 * SPDX-FileCopyrightText: (C) 2025 a dinosaur
 * SPDX-License-Identifier: Zlib
 */

#include <metal_stdlib>
#include <simd/simd.h>

struct VertexInput
{
	float3 position [[attribute(0)]];
};

struct VertexUniform
{
	metal::float4x4 viewproj;
};

struct Vertex2Fragment
{
	float4 position [[position]];
};

vertex Vertex2Fragment VertexMain(
	VertexInput in [[stage_in]],
	constant VertexUniform& u [[buffer(0)]])
{
	Vertex2Fragment out;
	out.position = u.viewproj * float4(in.position, 1.0);
	return out;
}

fragment half4 FragmentMain(Vertex2Fragment in [[stage_in]])
{
	return half4(1.0);
}
