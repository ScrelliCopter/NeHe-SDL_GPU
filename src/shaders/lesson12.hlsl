/*
 * SPDX-FileCopyrightText: (C) 2025 a dinosaur
 * SPDX-License-Identifier: Zlib
 */

struct VertexInput
{
	// Vertex
	float3 position : TEXCOORD0;
	float3 normal : TEXCOORD1;
	float2 texCoord : TEXCOORD2;
	float tint : TEXCOORD3;

	// Instance
	float4x4 model : TEXCOORD4;
	float4 color : TEXCOORD8;
};

cbuffer VertexUniform : register(b0, space1)
{
	float4x4 view : packoffset(c0);
	float4x4 projection : packoffset(c4);
};


struct Vertex2Pixel
{
	float4 position : SV_Position;
	float2 texCoord : TEXCOORD0;
	half4 color : COLOR0;
};

Vertex2Pixel VertexMain(VertexInput input)
{
	const float4x4 modelView = mul(view, transpose(input.model));
	const float3 normal = normalize(mul(modelView, float4(input.normal, 0.0))).xyz;

	const float3 lightDir = float3(0.0, 0.0, 1.0);
	const float lambert = max(0.0, dot(normal, lightDir));
	// Apply lighting as if we were in GL_COLOR_MATERIAL defaults
	const float lighting = input.tint * (0.2 + lambert);

	Vertex2Pixel output;
	output.position = mul(projection, mul(modelView, float4(input.position, 1.0)));
	output.texCoord = input.texCoord;
	output.color = half4(input.color) * half4(lighting, lighting, lighting, 1.0);
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
