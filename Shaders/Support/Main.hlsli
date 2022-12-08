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
#include "../brdf.hlsli"
#include "../subsurface_scattering_texturing_mode.hlsli"
#include "../subsurface_scattering_disney_transmittance.hlsli"

#define N_LIGHTS 5

#define PI 3.14159265358979323846

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
    float3 scatteringDistance;
    float padding_scatteringDistance;
    float3 transmittanceTint;
    float padding_transmittanceTint;
    float bumpiness;
    float specularIntensity;
    float specularRoughness;
    float specularFresnel;
    float specularlightEnabled;
    float skylightEnabled;
    float sssEnabled;
    float worldScale;
    float postscatterEnabled;
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

struct RenderV2P
{
    // Position and texcoord:
    float4 svPosition : SV_POSITION;
    float2 texcoord : TEXCOORD0;

    // For shading:
    centroid float3 worldPosition : TEXCOORD3;
    centroid float3 view : TEXCOORD4;
    centroid float3 normal : TEXCOORD5;
    centroid float3 tangent : TEXCOORD6;
};

RenderV2P RenderVS(float4 position
                   : POSITION0,
                     float3 normal
                   : NORMAL,
                     float3 tangent
                   : TANGENT,
                     float2 texcoord
                   : TEXCOORD0)
{
    RenderV2P output;

    // Transform to homogeneous projection space:
    output.svPosition = mul(position, currWorldViewProj);

    // Output texture coordinates:
    output.texcoord = texcoord;

    // Build the vectors required for shading:
    output.worldPosition = mul(position, world).xyz;
    output.view = cameraPosition - output.worldPosition;
    output.normal = mul(normal, (float3x3)worldInverseTranspose);
    output.tangent = mul(tangent, (float3x3)worldInverseTranspose);

    return output;
}

float3 UnpackNormalMap(float2 TextureSample);

float ShadowPCF(float3 worldPosition, int i, int samples, float width);

float4 RenderPS(
    RenderV2P input, 
    out float depth : SV_TARGET1, 
    out float4 albedoOut : SV_TARGET2, 
    out float4 sssTotalDiffuseReflectancePreScatterMultiplyFormFactorOut : SV_TARGET3
) : SV_TARGET0
{
    // We build the TBN frame here in order to be able to use the bump map for IBL:
    input.normal = normalize(input.normal);
    input.tangent = normalize(input.tangent);
    float3 bitangent = cross(input.normal, input.tangent);
    float3x3 tbn = transpose(float3x3(input.tangent, bitangent, input.normal));

    // Transform bumped normal to world space, in order to use IBL for ambient lighting:
    float3 tangentNormal = lerp(float3(0.0, 0.0, 1.0), UnpackNormalMap(normalTex.Sample(AnisotropicSampler, input.texcoord).gr), bumpiness);
    float3 normal = mul(tbn, tangentNormal);
    input.view = normalize(input.view);

    // Fetch albedo, specular parameters and static ambient occlusion:
    float4 albedoAndStrength = diffuseTex.Sample(AnisotropicSampler, input.texcoord);
    float3 specularAO = specularAOTex.Sample(LinearSampler, input.texcoord).rgb;
    float3 total_diffuse_reflectance_pre_scatter;
    [branch]
    if (sssEnabled > 0.0f)
    {
        total_diffuse_reflectance_pre_scatter = subsurface_scattering_total_diffuse_reflectance_pre_scatter_from_albedo((postscatterEnabled > 0.0), albedoAndStrength.rgb);
    }
    else
    {
        total_diffuse_reflectance_pre_scatter = albedoAndStrength.rgb;
    }
    float strength = albedoAndStrength.a;
    float occlusion = specularAO.b;
    float specularTint = specularAO.r * specularIntensity;
    float roughness = (specularAO.g / 0.3) * specularRoughness;

    // Initialize the output:
    float3 diffuseAccumulation = float3(0.0, 0.0, 0.0);
    float3 specularAccumulation = float3(0.0, 0.0, 0.0);

    for (int i = 0; i < N_LIGHTS; i++)
    {
        float3 light = normalize(lights[i].position - input.worldPosition);

        // Calculate attenuation:
        float light_attenuation;
        {
            float spot = dot(lights[i].direction, -light);
            if (spot > lights[i].falloffStart)
            {
                float dist = length(lights[i].position - input.worldPosition);
                float curve = min(pow(dist / lights[i].farPlane, 6.0), 1.0);
                float attenuation = lerp(1.0 / (1.0 + lights[i].color_attenuation.w * dist * dist), 0.0, curve);

                // And the spot light falloff:
                spot = saturate((spot - lights[i].falloffStart) / lights[i].falloffWidth);

                light_attenuation = attenuation * spot;
            }
            else
            {
                light_attenuation = 0.0f;
            }
        }

        if (light_attenuation > 0.0f)
        {
            // Add the diffuse and specular components:
            float ndotl = saturate(dot(light, normal));
            if (ndotl > 0.0f)
            {
                float3 halfn = normalize(input.view + light);
                float ndotv = saturate(dot(normal, input.view));
                float vdoth = saturate(dot(input.view, halfn));
                float ndoth = saturate(dot(normal, halfn));

                // And also the shadowing:
                float shadow = ShadowPCF(input.worldPosition, i, 3, 1.0);
                if (shadow > 0.0f)
                {
                    diffuseAccumulation += Diffuse_Disney(total_diffuse_reflectance_pre_scatter, roughness, ndotv, ndotl, vdoth) * ndotl * shadow * light_attenuation * lights[i].color_attenuation.xyz;

                    if (specularlightEnabled > 0.0f)
                    {
                        specularAccumulation += specularTint * Dual_Specular_TR(0.75, 1.30, 0.85, specularFresnel * float3(1.0, 1.0, 1.0), roughness, strength, ndotv, ndotl, ndoth, vdoth) * ndotl * shadow * light_attenuation * lights[i].color_attenuation.xyz;
                    }
                }
            }

            // Add the transmittance component:
            if (sssEnabled > 0.0f)
            {
                float transmittanceLightAttenuation = EvaluateTransmittanceLightAttenuation(input.normal, light);
                if (transmittanceLightAttenuation > 0.0)
                {
                    /**
                     * First we shrink the position inwards the surface to avoid artifacts:
                     * (Note that this can be done once for all the lights)
                     */
                    float metersPerUnit = worldScale;
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

                    // The shader code is merely to transform the thickness from world units to mm.
                    float thicknessInUnits = abs(d2 - d1);
                    float thicknessInMillimeters = 1000.0 * metersPerUnit * thicknessInUnits;

                    diffuseAccumulation += transmittanceTint * EvaluateTransmittance(scatteringDistance, thicknessInMillimeters) * transmittanceLightAttenuation * light_attenuation * lights[i].color_attenuation.xyz;
                }
            }
        }
    }

    // Add the ambient component:
    if (skylightEnabled > 0.0f)
    {
        diffuseAccumulation += total_diffuse_reflectance_pre_scatter * occlusion * ambient * irradianceTex.Sample(LinearSampler, normal).rgb;
    }

    // Store the ViewPositionZ value:
    depth = input.svPosition.z;

    // Store the Albedo and the SSS strength:
    albedoOut = albedoAndStrength;

    if (sssEnabled > 0.0f)
    {
        // Store the SSS 'total_diffuse_reflectance_pre_scatter * form_factor'
        sssTotalDiffuseReflectancePreScatterMultiplyFormFactorOut = float4(diffuseAccumulation, 1.0);

        return float4(specularAccumulation, 1.0);
    }
    else
    {
        sssTotalDiffuseReflectancePreScatterMultiplyFormFactorOut = float4(0.0, 0.0, 0.0, 1.0);

        return float4(diffuseAccumulation + specularAccumulation, 1.0);
    }
}

float3 UnpackNormalMap(float2 TextureSample)
{
    float2 NormalXY = TextureSample * float2(2.0f, 2.0f) + float2(-1.0f, -1.0f);
    float NormalZ = sqrt(saturate(1.0f - dot(NormalXY, NormalXY)));
    return float3(NormalXY.xy, NormalZ);
}

float ShadowPCF(float3 worldPosition, int i, int samples, float width)
{
    float4 shadowPosition = mul(float4(worldPosition, 1.0), lights[i].viewProjection);
    shadowPosition.xy /= shadowPosition.w;
    shadowPosition.z += lights[i].bias;

	float w;
	float h;
	ShadowMapArray_GetDimensions(i, w, h);

    float shadow = 0.0;
    float offset = (samples - 1.0) / 2.0;
    for (float x = -offset; x <= offset; x += 1.0)
    {
        for (float y = -offset; y <= offset; y += 1.0)
        {
            float2 pos = shadowPosition.xy + width * float2(x, y) / w;
			shadow += ShadowMapArray_SampleCmpLevelZero(i, pos, shadowPosition.z / lights[i].farPlane);
        }
    }
    shadow /= samples * samples;
    return shadow;
}
