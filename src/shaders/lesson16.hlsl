/*
 * SPDX-FileCopyrightText: (C) 2025 a dinosaur
 * SPDX-License-Identifier: Zlib
 */

struct VertexInput
{
	float3 position : TEXCOORD0;
	float2 texCoord : TEXCOORD1;
	float3 normal : TEXCOORD2;
};

struct Light
{
	float4 ambient;
	float4 diffuse;
	float4 position;
};

struct VertexUniform
{
	float4x4 modelView;
	float4x4 projection;
#ifdef LIGHTING
	Light light;
#endif
};

struct FogUniform
{
	float4 color;
	float density, start, end;
};

struct Vertex2Pixel
{
	float4 position : SV_Position;
	float2 texCoord : TEXCOORD0;
#ifdef LIGHTING
	half4 color : COLOR0;
#endif
	float eyeZ : DEPTH0;
};

ConstantBuffer<VertexUniform> ubo : register(b0, space1);
ConstantBuffer<FogUniform> fog : register(b0, space3);

Vertex2Pixel VertexMain(VertexInput input)
{
	const float4 position = mul(ubo.modelView, float4(input.position, 1.0));
#ifdef LIGHTING
	const float3 normal = normalize(mul(ubo.modelView, float4(input.normal, 0.0))).xyz;

	const float3 lightVec = ubo.light.position.xyz - position.xyz;
	const float lightDist2 = dot(lightVec, lightVec);
	const float3 dir = rsqrt(lightDist2) * lightVec;
	const float lambert = max(0.0, dot(normal, dir));

	const half3 ambient = 0.04 + 0.2 * half3(ubo.light.ambient.rgb);
	const half3 diffuse = 0.8 * half3(ubo.light.diffuse.rgb);
#endif

	Vertex2Pixel output;
	output.position = mul(ubo.projection, position);
	output.texCoord = input.texCoord;
#ifdef LIGHTING
	output.color = half4(ambient + lambert * diffuse, 1.0);
#endif
	output.eyeZ = -position.z;
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
#if defined(FOG_LINEAR)
	const float fogFact = saturate((fog.end - input.eyeZ) / (fog.end - fog.start));
#elif defined(FOG_EXP)
	const float fogFact = max(exp(-fog.density * input.eyeZ), 0.0);
#elif defined(FOG_EXP2)
	const float fogFact = exp(-(fog.density * fog.density * input.eyeZ * input.eyeZ));
#endif

	half4 color = Texture.Sample(Sampler, input.texCoord);
#ifdef LIGHTING
	color *= input.color;
#endif
	return fogFact * color + (1.0 - fogFact) * half4(fog.color);
}
