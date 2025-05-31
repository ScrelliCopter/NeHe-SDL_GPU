/*
 * SPDX-FileCopyrightText: (C) 2025 a dinosaur
 * SPDX-License-Identifier: Zlib
 */

#include <metal_stdlib>
#include <simd/simd.h>

struct VertexInput
{
	float3 position [[attribute(0)]];
	float2 texcoord [[attribute(1)]];
	float3 normal   [[attribute(2)]];
};

struct VertexUniform
{
	metal::float4x4 modelView;
	metal::float4x4 projection;
};

struct Light
{
    float4 ambient;
    float4 diffuse;
    float4 position;
};

struct Vertex2Fragment
{
	float4 position [[position]];
	float2 texcoord;
	half4 color;
};

vertex Vertex2Fragment VertexMain(
	VertexInput in [[stage_in]],
	constant VertexUniform& u [[buffer(0)]],
	constant Light& light [[buffer(1)]])
{
    const auto position = u.modelView * float4(in.position, 1.0);
    const auto normal = metal::normalize(u.modelView * float4(in.normal, 0.0)).xyz;

    const auto lightVec = light.position.xyz - position.xyz;
    const auto lightDist2 = metal::length_squared(lightVec);
    const auto dir = metal::rsqrt(lightDist2) * lightVec;
    const auto lambert = metal::max(0.0, metal::dot(normal, dir));

    const auto ambient = 0.04 + 0.2 * half3(light.ambient.rgb);
    const auto diffuse = 0.8 * half3(light.diffuse.rgb);

	Vertex2Fragment out;
	out.position = u.projection * position;
	out.texcoord = in.texcoord;
	out.color = half4(ambient + lambert * diffuse, 1.0);
	return out;
}

fragment half4 FragmentMain(
    Vertex2Fragment in [[stage_in]],
	metal::texture2d<half, metal::access::sample> texture [[texture(0)]],
	metal::sampler sampler [[sampler(0)]])
{
	return in.color * texture.sample(sampler, in.texcoord);
}
