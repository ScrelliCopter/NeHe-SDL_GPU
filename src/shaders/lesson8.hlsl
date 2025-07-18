/*
 * SPDX-FileCopyrightText: (C) 2025 a dinosaur
 * SPDX-License-Identifier: Zlib
 */

struct VertexInput
{
	float3 position : TEXCOORD0;
	float2 texcoord : TEXCOORD1;
};

struct VertexUniform
{
	float4x4 viewproj;
	float4 color;
};

struct Vertex2Pixel
{
	float4 position : SV_Position;
	float2 texcoord : TEXCOORD0;
	half4 color : COLOR0;
};

ConstantBuffer<VertexUniform> ubo : register(b0, space1);

Vertex2Pixel VertexMain(VertexInput input)
{
	Vertex2Pixel output;
	output.position = mul(ubo.viewproj, float4(input.position, 1.0));
	output.texcoord = input.texcoord;
	output.color = half4(ubo.color);
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
