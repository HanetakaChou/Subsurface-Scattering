/**
 * Copyright (C) 2012 Jorge Jimenez (jorge@iryoku.com). All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *    1. Redistributions of source code must retain the above copyright notice,
 *       this list of conditions and the following disclaimer.
 *
 *    2. Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS
 * IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL COPYRIGHT HOLDERS OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 * The views and conclusions contained in the software and documentation are
 * those of the authors and should not be interpreted as representing official
 * policies, either expressed or implied, of the copyright holders.
 */

SamplerState LinearSampler : register(s0);

SamplerState PointSampler : register(s1);

SamplerState AnisotropicSampler : register(s2);

SamplerComparisonState ShadowSampler : register(s3);

// And include our header!
#include "../Translucency.hlsli"

#define N_LIGHTS 5 

struct Light
{
	row_major float4x4 viewProjection;
	row_major float4x4 projection;
	float3 position;
	float3 direction;
	float4 color_attenuation;
	float falloffStart;
	float falloffWidth;
	float farPlane;
	float bias;
};

cbuffer UpdatedPerFrame : register(b0)
{
	float3 cameraPosition;
	row_major float4x4 currProj;
	Light lights[N_LIGHTS];
}

cbuffer UpdatedPerObject : register(b1)
{
	row_major float4x4 currWorldViewProj;
	row_major float4x4 world;
	row_major float4x4 worldInverseTranspose;
	float bumpiness;
	float specularIntensity;
	float specularRoughness;
	float specularFresnel;
	float translucency;
	float sssEnabled;
	float sssWidth;
	float ambient;
}

Texture2D diffuseTex : register(t0);
Texture2D normalTex : register(t1);
Texture2D specularAOTex : register(t3);
Texture2D beckmannTex : register(t4);
TextureCube irradianceTex : register(t5);
Texture2D shadowMaps[N_LIGHTS] : register(t6);

void ShadowMapArray_GetDimensions(float LightIndex, out float Width, out float Height)
{
	[branch]
	if (0 == LightIndex)
	{
		shadowMaps[0].GetDimensions(Width, Height);
	}
	else if (1 == LightIndex)
	{
		shadowMaps[1].GetDimensions(Width, Height);
	}
	else if (2 == LightIndex)
	{
		shadowMaps[2].GetDimensions(Width, Height);
	}
	else if (3 == LightIndex)
	{
		shadowMaps[3].GetDimensions(Width, Height);
	}
	else if (4 == LightIndex)
	{
		shadowMaps[4].GetDimensions(Width, Height);
	}
	else
	{
		// 5 == N_LIGHTS
		Width = 1.0;
		Height = 1.0;
	}
}

float ShadowMapArray_SampleCmpLevelZero(float LightIndex, float2 Location, float CompareValue)
{
	float tmp;
	[branch]
	if (0 == LightIndex)
	{
		tmp = shadowMaps[0].SampleCmpLevelZero(ShadowSampler, Location, CompareValue).r;
	}
	else if (1 == LightIndex)
	{
		tmp = shadowMaps[1].SampleCmpLevelZero(ShadowSampler, Location, CompareValue).r;
	}
	else if (2 == LightIndex)
	{
		tmp = shadowMaps[2].SampleCmpLevelZero(ShadowSampler, Location, CompareValue).r;
	}
	else if (3 == LightIndex)
	{
		tmp = shadowMaps[3].SampleCmpLevelZero(ShadowSampler, Location, CompareValue).r;
	}
	else if (4 == LightIndex)
	{
		tmp = shadowMaps[4].SampleCmpLevelZero(ShadowSampler, Location, CompareValue).r;
	}
	else
	{
		// 5 == N_LIGHTS
		tmp = 0.0;
	}
	return tmp;
}

float ShadowMapArray_SampleLevel(float LightIndex, float2 Location)
{
	float tmp;
	[branch]
	if (0 == LightIndex)
	{
		tmp = shadowMaps[0].SampleLevel(LinearSampler, Location, 0.0).r;
	}
	else if (1 == LightIndex)
	{
		tmp = shadowMaps[1].SampleLevel(LinearSampler, Location, 0.0).r;
	}
	else if (2 == LightIndex)
	{
		tmp = shadowMaps[2].SampleLevel(LinearSampler, Location, 0.0).r;
	}
	else if (3 == LightIndex)
	{
		tmp = shadowMaps[3].SampleLevel(LinearSampler, Location, 0.0).r;
	}
	else if (4 == LightIndex)
	{
		tmp = shadowMaps[4].SampleLevel(LinearSampler, Location, 0.0).r;
	}
	else
	{
		// 5 == N_LIGHTS
		tmp = 0.0;
	}
	return tmp;
}

struct RenderV2P {
	// Position and texcoord:
	float4 svPosition : SV_POSITION;
	float2 texcoord : TEXCOORD0;

	// For shading:
	centroid float3 worldPosition : TEXCOORD3;
	centroid float3 view : TEXCOORD4;
	centroid float3 normal : TEXCOORD5;
	centroid float3 tangent : TEXCOORD6;
};

RenderV2P RenderVS(float4 position : POSITION0,
	float3 normal : NORMAL,
	float3 tangent : TANGENT,
	float2 texcoord : TEXCOORD0) {
	RenderV2P output;

	// Transform to homogeneous projection space:
	output.svPosition = mul(position, currWorldViewProj);

	// Output texture coordinates:
	output.texcoord = texcoord;

	// Build the vectors required for shading:
	output.worldPosition = mul(position, world).xyz;
	output.view = cameraPosition - output.worldPosition;
	output.normal = mul(normal, (float3x3) worldInverseTranspose);
	output.tangent = mul(tangent, (float3x3) worldInverseTranspose);

	return output;
}


float3 BumpMap(Texture2D normalTex, float2 texcoord) {
	float3 bump;
	bump.xy = -1.0 + 2.0 * normalTex.Sample(AnisotropicSampler, texcoord).gr;
	bump.z = sqrt(1.0 - bump.x * bump.x - bump.y * bump.y);
	return normalize(bump);
}


float Fresnel(float3 half, float3 view, float f0) {
	float base = 1.0 - dot(view, half);
	float exponential = pow(base, 5.0);
	return exponential + f0 * (1.0 - exponential);
}

float SpecularKSK(Texture2D beckmannTex, float3 normal, float3 light, float3 view, float roughness) {
	float3 half = view + light;
	float3 halfn = normalize(half);

	float ndotl = max(dot(normal, light), 0.0);
	float ndoth = max(dot(normal, halfn), 0.0);

	float ph = pow(2.0 * beckmannTex.SampleLevel(LinearSampler, float2(ndoth, roughness), 0).r, 10.0);
	float f = lerp(0.25, Fresnel(halfn, view, 0.028), specularFresnel);
	float ksk = max(ph * f / dot(half, half), 0.0);

	return ndotl * ksk;
}

float ShadowPCF(float3 worldPosition, int i, int samples, float width) {
	float4 shadowPosition = mul(float4(worldPosition, 1.0), lights[i].viewProjection);
	shadowPosition.xy /= shadowPosition.w;
	shadowPosition.z += lights[i].bias;

	float w;
	float h;
	ShadowMapArray_GetDimensions(i, w, h);

	float shadow = 0.0;
	float offset = (samples - 1.0) / 2.0;
	[unroll]
	for (float x = -offset; x <= offset; x += 1.0) {
		[unroll]
		for (float y = -offset; y <= offset; y += 1.0) 
		{
			float2 pos = shadowPosition.xy + width * float2(x, y) / w;
			shadow += ShadowMapArray_SampleCmpLevelZero(i, pos, shadowPosition.z / lights[i].farPlane);
		}
	}
	shadow /= samples * samples;
	return shadow;
}

#define HAS_SPECULAR 1
#define HAS_SKYLIGHT 1

float4 RenderPS(RenderV2P input,
	out float depth : SV_TARGET1,
	out float4 specularColor : SV_TARGET3) : SV_TARGET0{
		// We build the TBN frame here in order to be able to use the bump map for IBL:
		input.normal = normalize(input.normal);
		input.tangent = normalize(input.tangent);
		float3 bitangent = cross(input.normal, input.tangent);
		float3x3 tbn = transpose(float3x3(input.tangent, bitangent, input.normal));

		// Transform bumped normal to world space, in order to use IBL for ambient lighting:
		float3 tangentNormal = lerp(float3(0.0, 0.0, 1.0), BumpMap(normalTex, input.texcoord), bumpiness);
		float3 normal = mul(tbn, tangentNormal);
		input.view = normalize(input.view);

		// Fetch albedo, specular parameters and static ambient occlusion:
		float4 albedo = diffuseTex.Sample(AnisotropicSampler, input.texcoord);
	#if HAS_SPECULAR == 1 || HAS_SKYLIGHT == 1
		float3 specularAO = specularAOTex.Sample(LinearSampler, input.texcoord).rgb;
	#endif

	#if HAS_SKYLIGHT == 1
		float occlusion = specularAO.b;
	#endif

	#if HAS_SPECULAR == 1 
		float intensity = specularAO.r * specularIntensity;
		float roughness = (specularAO.g / 0.3) * specularRoughness;
	#endif

		// Initialize the output:
		float4 color = float4(0.0, 0.0, 0.0, 0.0);
		specularColor = float4(0.0, 0.0, 0.0, 0.0);

		for (int i = 0; i < N_LIGHTS; i++) {
			float3 light = lights[i].position - input.worldPosition;
			float dist = length(light);
			light /= dist;

			float spot = dot(lights[i].direction, -light);
			[flatten]
			if (spot > lights[i].falloffStart) {
				// Calculate attenuation:
				float curve = min(pow(dist / lights[i].farPlane, 6.0), 1.0);
				float attenuation = lerp(1.0 / (1.0 + lights[i].color_attenuation.w * dist * dist), 0.0, curve);

				// And the spot light falloff:
				spot = saturate((spot - lights[i].falloffStart) / lights[i].falloffWidth);

				// Calculate some terms we will use later on:
				float3 f1 = lights[i].color_attenuation.xyz * attenuation * spot;
				float3 f2 = albedo.rgb * f1;

				// Calculate the diffuse and specular lighting:
				float3 diffuse = saturate(dot(light, normal));
	#if HAS_SPECULAR == 1
				float specular = intensity * SpecularKSK(beckmannTex, normal, light, input.view, roughness);
	#endif

				// And also the shadowing:
				float shadow = ShadowPCF(input.worldPosition, i, 3, 1.0);

				// Add the diffuse and specular components:
				#ifdef SEPARATE_SPECULARS
				color.rgb += shadow * f2 * diffuse;
	#if HAS_SPECULAR == 1
				specularColor.rgb += shadow * f1 * specular;
	#endif
				#else
				color.rgb += shadow * (f2 * diffuse + f1 * specular);
				#endif

				// Add the transmittance component:
				if (sssEnabled > 0.0f)
				{
					/**
					* First we shrink the position inwards the surface to avoid artifacts:
					* (Note that this can be done once for all the lights)
					*/
					float metersPerUnit = 3.0 * 0.001 * 0.5 / sssWidth;
					float4 shrinkedPos = float4(input.worldPosition - 0.000625 / metersPerUnit * input.normal, 1.0);

					// Faceworks
					// g_deepScatterNormalOffset = -0.0007f

					/**
					 * Now we calculate the thickness from the light point of view:
					 */
					float4 shadowPosition = mul(shrinkedPos, lights[i].viewProjection);
					shadowPosition /= shadowPosition.w;
					float d1 = lights[i].projection[3][2] / (ShadowMapArray_SampleLevel(i, shadowPosition.xy) - lights[i].projection[2][2]);
					float d2 = lights[i].projection[3][2] / (shadowPosition.z - lights[i].projection[2][2]);
					float worldDistance = abs(d2 - d1);

					color.rgb += f2 * SSSSTransmittance(translucency, sssWidth, input.normal, light, worldDistance);
				}
			}
		}

		// Add the ambient component:
	#if HAS_SKYLIGHT == 1
		color.rgb += occlusion * ambient * albedo.rgb * irradianceTex.Sample(LinearSampler, normal).rgb;
	#endif

		// Store the SSS strength:
		specularColor.a = albedo.a;

		// Store the ViewPositionZ value:
		depth = currProj[3][2] / (input.svPosition.z - currProj[2][2]);

		// Store the SSS strength:
		color.a = albedo.a;

		return color;
}
