/*
 * SPDX-FileCopyrightText: (C) 2025 a dinosaur
 * SPDX-License-Identifier: Zlib
 */

struct InstanceInput
{
	float3 position : TEXCOORD0;
	float3 color : TEXCOORD1;
	float2 angle : TEXCOORD2;
};
struct VertexUniform
{
	float4x4 view, projection;
};

struct Vertex2Pixel
{
	float4 position : SV_Position;
	float2 texcoord : TEXCOORD0;
	half4 color : COLOR0;
};

ConstantBuffer<VertexUniform> ubo : register(b0, space1);

static const float2 quadTexCoords[4] =
{
	float2(0.0, 0.0),
	float2(1.0, 0.0),
	float2(1.0, 1.0),
	float2(0.0, 1.0)
};

static const min12int quadIndices[6] =
{
	0, 1, 2,
	2, 3, 0
};

Vertex2Pixel VertexMain(uint vertexID : SV_VertexID, InstanceInput input)
{
	const float3 quadVertices[4] =
	{
		{ -input.angle.x + input.angle.y, -input.angle.x - input.angle.y, 0.0 },
		{  input.angle.x + input.angle.y, -input.angle.x + input.angle.y, 0.0 },
		{  input.angle.x - input.angle.y,  input.angle.x + input.angle.y, 0.0 },
		{ -input.angle.x - input.angle.y,  input.angle.x - input.angle.y, 0.0 }
	};

	const uint quadIndex = quadIndices[vertexID];

	float3 position = mul(transpose((float3x3)ubo.view), quadVertices[quadIndex]);
	position += input.position;

	Vertex2Pixel output;
	output.position = mul(ubo.projection, mul(ubo.view, float4(position, 1.0)));
	output.color = half4(input.color, 1.0);
	output.texcoord = quadTexCoords[quadIndex];
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
