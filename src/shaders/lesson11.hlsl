/*
 * SPDX-FileCopyrightText: (C) 2025 a dinosaur
 * SPDX-License-Identifier: Zlib
 */

struct VertexInput
{
	float2 position : TEXCOORD0;
	float2 texCoord : TEXCOORD1;
};

cbuffer VertexUniform : register(b0, space1)
{
	float4x4 viewProj : packoffset(c0);
	float waveOffset : packoffset(c4);
};

struct Vertex2Pixel
{
	float4 position : SV_Position;
	float2 texCoord : TEXCOORD0;
};

Vertex2Pixel VertexMain(VertexInput input)
{
	const float rescale = 44.0 / 45.0, tau = 6.283185308;
	const float z = sin((input.texCoord.x * rescale + waveOffset) * tau);
	const float4 position = float4(input.position, z, 1.0);

	Vertex2Pixel output;
	output.position = mul(viewProj, position);
	output.texCoord = input.texCoord;
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
	return Texture.Sample(Sampler, input.texCoord);
}
