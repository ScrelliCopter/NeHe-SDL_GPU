/*
 * SPDX-FileCopyrightText: (C) 2025 a dinosaur
 * SPDX-License-Identifier: Zlib
 */

#include <metal_stdlib>
#include <simd/simd.h>

struct VertexInput
{
	float3 position [[attribute(0)]];
	float2 texCoord [[attribute(1)]];
	float3 normal   [[attribute(2)]];
};

struct Light
{
	float4 ambient;
	float4 diffuse;
	float4 position;
};

struct VertexUniform
{
	metal::float4x4 modelView;
	metal::float4x4 projection;
#ifdef LIGHTING
	Light light;
#endif
};

struct Fog
{
	float4 color;
	float density, start, end;
};

struct FragmentUniform { Fog fog; };

struct Vertex2Fragment
{
	float4 position [[position]];
	float2 texCoord;
#ifdef LIGHTING
	half4 color;
#endif
	float eyeZ;
};

vertex Vertex2Fragment VertexMain(
	VertexInput in [[stage_in]],
	constant VertexUniform& u [[buffer(0)]])
{
	const auto position = u.modelView * float4(in.position, 1.0);
#ifdef LIGHTING
	const auto normal = metal::normalize(u.modelView * float4(in.normal, 0.0)).xyz;

	const auto lightVec = u.light.position.xyz - position.xyz;
	const auto lightDist2 = metal::length_squared(lightVec);
	const auto dir = metal::rsqrt(lightDist2) * lightVec;
	const auto lambert = metal::max(0.0, metal::dot(normal, dir));

	const auto ambient = 0.04 + 0.2 * half3(u.light.ambient.rgb);
	const auto diffuse = 0.8 * half3(u.light.diffuse.rgb);
#endif

	Vertex2Fragment out;
	out.position = u.projection * position;
	out.texCoord = in.texCoord;
#ifdef LIGHTING
	out.color = half4(ambient + lambert * diffuse, 1.0);
#endif
	out.eyeZ = -position.z;
	return out;
}

fragment half4 FragmentMain(
	Vertex2Fragment in [[stage_in]],
	constant FragmentUniform& u [[buffer(0)]],
	metal::texture2d<half, metal::access::sample> texture [[texture(0)]],
	metal::sampler sampler [[sampler(0)]])
{
#ifdef FOG_LINEAR
	const auto fogFact = metal::saturate((u.fog.end - in.eyeZ) / (u.fog.end - u.fog.start));
#elifdef FOG_EXP
	const auto fogFact = metal::max(metal::exp(-u.fog.density * in.eyeZ), 0.0);
#elifdef FOG_EXP2
	const auto fogFact = metal::exp(-(u.fog.density * u.fog.density * in.eyeZ * in.eyeZ));
#endif

	half4 color = texture.sample(sampler, in.texCoord);
#ifdef LIGHTING
	color *= in.color;
#endif
	return fogFact * color + (1.0 - fogFact) * half4(u.fog.color);
}
