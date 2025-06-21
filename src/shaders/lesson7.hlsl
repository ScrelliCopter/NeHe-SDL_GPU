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

struct VertexUniform
{
	float4x4 modelView;
	float4x4 projection;
};

struct LightUniform
{
	float4 ambient;
	float4 diffuse;
	float4 position;
};

struct Vertex2Pixel
{
	float4 position : SV_Position;
	float2 texcoord : TEXCOORD0;
	half4 color : COLOR0;
};

ConstantBuffer<VertexUniform> ubo : register(b0, space1);
ConstantBuffer<LightUniform> light : register(b1, space1);

Vertex2Pixel VertexMain(VertexInput input)
{
	const float4 position = mul(ubo.modelView, float4(input.position, 1.0));
	const float3 normal = normalize(mul(ubo.modelView, float4(input.normal, 0.0))).xyz;

	const float3 lightVec = light.position.xyz - position.xyz;
	const float lightDist2 = dot(lightVec, lightVec);
	const float3 dir = rsqrt(lightDist2) * lightVec;
	const float lambert = max(0.0, dot(normal, dir));

	const half3 ambient = 0.04 + 0.2 * half3(light.ambient.rgb);
	const half3 diffuse = 0.8 * half3(light.diffuse.rgb);

	Vertex2Pixel output;
	output.position = mul(ubo.projection, position);
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
