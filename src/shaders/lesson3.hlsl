/*
 * SPDX-FileCopyrightText: (C) 2025 a dinosaur
 * SPDX-License-Identifier: Zlib
 */

struct VertexInput
{
	float3 position : TEXCOORD0;
	float4 color : TEXCOORD1;
};

cbuffer VertexUniform : register(b0, space1)
{
	float4x4 viewproj : packoffset(c0);
};

struct Vertex2Pixel
{
	float4 position : SV_Position;
	half4 color : COLOR0;
};

Vertex2Pixel VertexMain(VertexInput input)
{
	Vertex2Pixel output;
	output.position = mul(viewproj, float4(input.position, 1.0));
	output.color = half4(input.color);
	return output;
}

#ifdef VULKAN
half4 FragmentMain(Vertex2Pixel input) : SV_Target0
#else
half4 PixelMain(Vertex2Pixel input) : SV_Target0
#endif
{
	return input.color;
}
