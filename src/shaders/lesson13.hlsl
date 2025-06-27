/*
 * SPDX-FileCopyrightText: (C) 2025 a dinosaur
 * SPDX-License-Identifier: Zlib
 */

struct CharacterInput
{
	float4 src : TEXCOORD0, dst : TEXCOORD1;
	uint vertexID : SV_VertexID;
};

struct VertexUniform
{
	float4x4 modelViewProj;
	float4 color;
};

struct Vertex2Pixel
{
	float4 position : SV_Position;
	float2 texCoord : TEXCOORD0;
	half4 color : COLOR0;
};

ConstantBuffer<VertexUniform> ubo : register(b0, space1);

Vertex2Pixel VertexMain(CharacterInput input)
{
	const float2 offset = float2(
		input.vertexID >>  1,
		input.vertexID & 0x1);
	const float2 position = input.dst.xy + input.dst.zw * offset;

	Vertex2Pixel output;
	output.position = mul(ubo.modelViewProj, float4(position, 0.0, 1.0));
	output.texCoord = input.src.xy + input.src.zw * offset;
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
	return input.color * Texture.Sample(Sampler, input.texCoord).a;
}
