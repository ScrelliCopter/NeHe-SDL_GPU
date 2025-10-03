/*
 * SPDX-FileCopyrightText: (C) 2025 a dinosaur
 * SPDX-License-Identifier: Zlib
 */

struct CharacterInput
{
	float3 color : TEXCOORD0;
	float2 position : TEXCOORD1;
	int character : TEXCOORD2;
};

struct VertexUniform
{
	float4x4 modelViewProj;
};

struct Vertex2Pixel
{
	float4 position : SV_Position;
	float2 texCoord : TEXCOORD0;
	half4 color : COLOR0;
};

ConstantBuffer<VertexUniform> ubo : register(b0, space1);

Vertex2Pixel VertexMain(CharacterInput input, uint vertexID : SV_VertexID)
{
	const float2 offset = float2(vertexID >> 1, 1 - (vertexID & 0x1));
	const float2 source = float2(input.character & 0xF, 15 - (input.character >> 4));

	Vertex2Pixel output;
	output.position = mul(ubo.modelViewProj, float4(input.position + 16.0 * offset, 0.0, 1.0));
	output.texCoord = 0.0625 * (source + offset);
	output.color = half4(half3(input.color), 1.0);
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
	return input.color * Texture.Sample(Sampler, input.texCoord);
}
