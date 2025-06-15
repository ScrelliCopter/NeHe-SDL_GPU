/*
 * SPDX-FileCopyrightText: (C) 2025 a dinosaur
 * SPDX-License-Identifier: Zlib
 */

struct VertexInput
{
	float3 position : TEXCOORD0;
	float2 texcoord : TEXCOORD1;
	float3 normal : TEXCOORD2;
};

cbuffer VertexUniform : register(b0, space1)
{
	float4x4 modelView : packoffset(c0);
	float4x4 projection : packoffset(c4);
};

cbuffer LightUniform : register(b1, space1)
{
	float4 lightAmbient : packoffset(c0);
	float4 lightDiffuse : packoffset(c1);
	float4 lightPosition : packoffset(c2);
};

struct Vertex2Pixel
{
	float4 position : SV_Position;
	float2 texcoord : TEXCOORD0;
	half4 color : COLOR0;
};

Vertex2Pixel VertexMain(VertexInput input)
{
	const float4 position = mul(modelView, float4(input.position, 1.0));
	const float3 normal = normalize(mul(modelView, float4(input.normal, 0.0))).xyz;

	const float3 lightVec = lightPosition.xyz - position.xyz;
	const float lightDist2 = dot(lightVec, lightVec);
	const float3 dir = rsqrt(lightDist2) * lightVec;
	const float lambert = max(0.0, dot(normal, dir));

	const half3 ambient = 0.04 + 0.2 * half3(lightAmbient.rgb);
	const half3 diffuse = 0.8 * half3(lightDiffuse.rgb);

	Vertex2Pixel output;
	output.position = mul(projection, position);
	output.texcoord = input.texcoord;
	output.color = half4(ambient + lambert * diffuse, 1.0);
	return output;
}

Texture2D<half4> Texture : register(t0, space2);
SamplerState Sampler : register(s0, space2);

#ifdef VULKAN
half4 FragmentMain(Vertex2Pixel input) : SV_Target0
#else
half4 PixelMain(Vertex2Pixel input) : SV_Target0
#endif
{
	return input.color * Texture.Sample(Sampler, input.texcoord);
}
