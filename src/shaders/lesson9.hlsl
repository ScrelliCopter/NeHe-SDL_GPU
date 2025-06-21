/*
 * SPDX-FileCopyrightText: (C) 2025 a dinosaur
 * SPDX-License-Identifier: Zlib
 */

struct VertexInput
{
	// Vertex
	float3 position : TEXCOORD0;
	float2 texcoord : TEXCOORD1;

	// Instance
	float4x4 model : TEXCOORD2;
	float4 color : TEXCOORD6;
};

struct VertexInstance
{
	float4x4 model;
	float4 color;
};

struct VertexUniform
{
	float4x4 projection;
};

struct Vertex2Pixel
{
	float4 position : SV_Position;
	float2 texcoord : TEXCOORD0;
	half4 color : COLOR0;
};

ConstantBuffer<VertexUniform> ubo : register(b0, space1);
StructuredBuffer<VertexInstance> Instances : register(t0, space0);

Vertex2Pixel VertexMain(VertexInput input)
{
#ifdef VULKAN
	const float4x4 model = transpose(input.model);
#else
	const float4x4 model = input.model;
#endif
	const float4x4 modelViewProj = mul(ubo.projection, model);

	Vertex2Pixel output;
	output.position = mul(modelViewProj, float4(input.position, 1.0));
	output.color = half4(input.color);
	output.texcoord = input.texcoord;
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
