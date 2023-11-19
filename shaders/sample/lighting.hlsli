//----------------------------------------------------------------------------------
// File:        FaceWorks/samples/sample_d3d11/shaders/lighting.hlsli
// SDK Version: v1.0
// Email:       gameworks@nvidia.com
// Site:        http://developer.nvidia.com/
//
// Copyright (c) 2014-2016, NVIDIA CORPORATION. All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions
// are met:
//  * Redistributions of source code must retain the above copyright
//    notice, this list of conditions and the following disclaimer.
//  * Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the distribution.
//  * Neither the name of NVIDIA CORPORATION nor the names of its
//    contributors may be used to endorse or promote products derived
//    from this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ``AS IS'' AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
// PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
// EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
// PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
// OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
//----------------------------------------------------------------------------------

#ifndef LIGHTING_HLSLI
#define LIGHTING_HLSLI

#include "common.hlsli"
#include "tonemap.hlsli"
#include "../GFSDK_FaceWorks.hlsli"

// Normal mapping

float3 UnpackNormal(
	float3 sample,
	float normalStrength)
{
	return lerp(float3(0, 0, 1),
				sample * 2.0 - 1.0,
				normalStrength);
}

// Shadow filtering, using variance shadow maps

float EvaluateShadowVSM(
	float4 uvzwShadow,
	float3 normalGeom)
{
	float3 uvzShadow = uvzwShadow.xyz / uvzwShadow.w;

	float2 vsmValue = g_texVSM.Sample(g_ssBilinearClamp, uvzShadow.xy);
	float mean = vsmValue.x;
	float variance = max(g_vsmMinVariance, vsmValue.y - mean * mean);

	return saturate(variance / (variance + square(uvzShadow.z - mean)));
}

// Diffuse lighting

float3 EvaluateDiffuseLight(
	float3 normalGeom,
	float3 normalShade,
	float shadow)
{
	// Directional light diffuse
	float NdotL = saturate(dot(normalShade, g_vecDirectionalLight));
	float3 rgbLightDiffuse = g_rgbDirectionalLight * (NdotL * shadow);

	// IBL diffuse
	rgbLightDiffuse += g_texCubeDiffuse.Sample(g_ssTrilinearRepeat, normalShade);

	return rgbLightDiffuse;
}

float3 EvaluateSSSDiffuseLight(
	float3 normalGeom,
	float3 normalShade,
	float3 normalBlurred,
	float shadow,
	float curvature,
	GFSDK_FaceWorks_CBData faceworksData)
{
	// Directional light diffuse
	float3 rgbSSS = GFSDK_FaceWorks_EvaluateSSSDirectLight(
		faceworksData,
		normalGeom, normalShade, normalBlurred,
		g_vecDirectionalLight, curvature,
		g_texCurvatureLUT, g_ssBilinearClamp);
	float3 rgbShadow = GFSDK_FaceWorks_EvaluateSSSShadow(
		faceworksData,
		normalGeom, g_vecDirectionalLight, shadow,
		g_texShadowLUT, g_ssBilinearClamp);
	float3 rgbLightDiffuse = g_rgbDirectionalLight * rgbSSS * rgbShadow;

	// IBL diffuse
	float3 normalAmbient0, normalAmbient1, normalAmbient2;
	GFSDK_FaceWorks_CalculateNormalsForAmbientLight(
		normalShade, normalBlurred,
		normalAmbient0, normalAmbient1, normalAmbient2);
	float3 rgbAmbient0 = g_texCubeDiffuse.Sample(g_ssTrilinearRepeat, normalAmbient0);
	float3 rgbAmbient1 = g_texCubeDiffuse.Sample(g_ssTrilinearRepeat, normalAmbient1);
	float3 rgbAmbient2 = g_texCubeDiffuse.Sample(g_ssTrilinearRepeat, normalAmbient2);
	rgbLightDiffuse += GFSDK_FaceWorks_EvaluateSSSAmbientLight(
		rgbAmbient0, rgbAmbient1, rgbAmbient2);

	return rgbLightDiffuse;
}

// Specular lighting

// https://oeis.org/A000796
#define PI 3.14159265358979323846264338327950288419716939937510582097494459230781640628620899862803482534211706798214

float3 F_Schlick(float3 f0, float f90, float LdotH)
{
	// Equation 9.18 of Real-Time Rendering Fourth Edition
	// U3D: [F_Schlick](https://github.com/Unity-Technologies/Graphics/blob/v10.8.0/com.unity.render-pipelines.core/ShaderLibrary/BSDF.hlsl#L45)
	float x = 1.0 - LdotH;
	float x2 = x * x;
	float x5 = x2 * x2 * x;
	return (f0 * (1.0 - x5) + float3(f90, f90, f90) * x5);
}

float3 F_Schlick(float3 f0, float LdotH)
{
	// UE: [F_Schlick](https://github.com/EpicGames/UnrealEngine/blob/4.27/Engine/Shaders/Private/BRDF.ush#L415)
	// Anything less than 2% is physically impossible and is instead considered to be shadowing
	float f90 = clamp(50.0 * f0.g, 0.0, 1.0);

	return F_Schlick(f0, f90, LdotH);
}

float D_Blinn(float n, float NdotH)
{
	// UE: [D_Blinn](https://github.com/EpicGames/UnrealEngine/blob/4.27/Engine/Shaders/Private/BRDF.ush#L303)
	return ((n + 2.0) / (2.0 * PI) * pow(NdotH, n));
}

float Vis_Schlick(float n, float NdotV, float NdotL)
{
	// UE: [Vis_Schlick](https://github.com/EpicGames/UnrealEngine/blob/4.27/Engine/Shaders/Private/BRDF.ush#L361)
	float k = rsqrt(n * (PI * 0.25) + (PI * 0.5));
	float Vis_SchlickV = k + NdotV * (1.0 - k); // lerp(k, 1.0, NdotV)
	float Vis_SchlickL = k + NdotL * (1.0 - k); // lerp(k, 1.0, NdotL)
	return (0.25 / (Vis_SchlickV * Vis_SchlickL));
}

float3 Specular_Blinn(float3 specular_color, float n, float NdotV, float NdotL, float NdotH, float LdotH)
{
	float D = D_Blinn(n, NdotH);
	float BRDF_V = Vis_Schlick(n, NdotV, NdotL);
	float3 F = F_Schlick(specular_color, LdotH);
	return (D * BRDF_V) * F;
}

float D_TR(float alpha, float NdotH)
{
	// Trowbridge-Reitz

	// Equation 9.41 of Real-Time Rendering Fourth Edition: "Although ¡°Trowbridge-Reitz distribution¡± is technically the correct name"
	// Equation 8.11 of PBR Book: https://pbr-book.org/3ed-2018/Reflection_Models/Microfacet_Models#MicrofacetDistributionFunctions
	float alpha2 = alpha * alpha;
	float denominator = 1.0 + NdotH * (NdotH * alpha2 - NdotH);
	return (1.0 / PI) * (alpha2 / (denominator * denominator));
}

float V_HC_TR(float alpha, float NdotV, float NdotL)
{
	// Height-Correlated Trowbridge-Reitz

	// Lambda:
	// Equation 8.13 of PBR Book: https://pbr-book.org/3ed-2018/Reflection_Models/Microfacet_Models#MaskingandShadowing

	// Lambda for Trowbridge-Reitz:
	// Equation 9.42 of Real-Time Rendering Fourth Edition
	// Figure 8.18 of PBR Book: https://pbr-book.org/3ed-2018/Reflection_Models/Microfacet_Models#MaskingandShadowing
	// ¦«(V) = 0.5*(-1.0 + (1.0/NoV)*sqrt(alpha^2 + (1.0 - alpha^2)*NoV^2))
	// ¦«(L) = 0.5*(-1.0 + (1.0/NoL)*sqrt(alpha^2 + (1.0 - alpha^2)*NoL^2))

	// G2
	// Equation 9.31 of Real-Time Rendering Fourth Edition
	// PBR Book / 8.4.3 Masking and Shadowing: "A more accurate model can be derived assuming that microfacet visibility is more likely the higher up a given point on a microface"
	// G2 = 1.0/(1.0 + ¦«(V) + ¦«(L)) = (2.0*NoV*NoL)/(NoL*sqrt(alpha^2 + (1.0 - alpha^2)*NoV^2) + NoV*sqrt(alpha^2 + (1.0 - alpha^2)*NoL^2))

	// V = G2/(4.0*NoV*NoL) = 0.5/(NoL*sqrt(alpha^2 + (1.0 - alpha^2)*NoV^2) + NoV*sqrt(alpha^2 + (1.0 - alpha^2)*NoL^2))

	float alpha2 = alpha * alpha;
	float term_v = NdotL * sqrt(alpha2 + (1.0 - alpha2) * NdotV * NdotV);
	float term_L = NdotV * sqrt(alpha2 + (1.0 - alpha2) * NdotL * NdotL);
	// UE: [Vis_SmithJointApprox](https://github.com/EpicGames/UnrealEngine/blob/4.27/Engine/Shaders/Private/BRDF.ush#L380)
	// highp float term_v = NdotL * (alpha + (1.0 - alpha) * NdotV);
	// highp float term_L = NdotV * (alpha + (1.0 - alpha) * NdotL);
	return (0.5 / (term_v + term_L));
}

float3 Specular_TR(float3 specular_color, float roughness, float NdotV, float NdotL, float NdotH, float LdotH)
{
	// Real-Time Rendering Fourth Edition / 9.8.1 Normal Distribution Functions: "In the Disney principled shading model, Burley[214] exposes the roughness control to users as ¦Ág = r2, where r is the user-interface roughness parameter value between 0 and 1."
	float alpha = roughness * roughness;

	float D = D_TR(alpha, NdotH);
	float BRDF_V = V_HC_TR(alpha, NdotV, NdotL);
	float3 F = F_Schlick(specular_color, LdotH);
	return (D * BRDF_V) * F;
}

float3 EvaluateSpecularLight(
	float3 normalGeom,
	float3 normalShade,
	float3 vecCamera,
	float specReflectance,
	float gloss,
	float shadow)
{
	float specLobeBlend = 0.05;
	float3 specular_color = float3(specReflectance, specReflectance, specReflectance);
	float NdotV = saturate(dot(normalShade, vecCamera));

	// Directional light spec
	float3 rgbDirectionalLightSpecular;
	{
		float3 vecHalf = normalize(g_vecDirectionalLight + vecCamera);
		float NdotL = saturate(dot(normalShade, g_vecDirectionalLight));
		float NdotH = saturate(dot(normalShade, vecHalf));
		float LdotH = dot(g_vecDirectionalLight, vecHalf);

		// Evaluate NDF and visibility function:

		// Double gloss on second lobe
		float specPower = exp2(gloss * 13.0);
		float specPower0 = specPower;
		float specPower1 = square(specPower);
#if 0
		// Two-lobe Blinn-Phong 
		float3 specResult0 = Specular_Blinn(specular_color, specPower0, NdotV, NdotL, NdotH, LdotH);
		float3 specResult1 = Specular_Blinn(specular_color, specPower1, NdotV, NdotL, NdotH, LdotH);
		float3 specResult = lerp(specResult0, specResult1, specLobeBlend);
#else
		// Dual Specular TR
				
		// D_Blinn: alpha = sqrt(2.0 / (n + 2.0));
		// Vis_Schlick: alpha = sqrt(4.0 / (n * (PI * 0.25) + (PI * 0.5)));
		float alpha0_D_Blinn = sqrt(2.0 / (specPower0 + 2.0));
		float alpha0_Vis_Schlick = sqrt(4.0 / (specPower0 * (PI * 0.25) + (PI * 0.5)));
		float roughtness0 = sqrt(0.5 * (alpha0_D_Blinn + alpha0_Vis_Schlick));
		float alpha1_D_Blinn = sqrt(2.0 / (specPower1 + 2.0));
		float alpha1_Vis_Schlick = sqrt(4.0 / (specPower1 * (PI * 0.25) + (PI * 0.5)));
		float roughtness1 = sqrt(0.5 * (alpha1_D_Blinn + alpha1_Vis_Schlick));

		// UE: [DualSpecularGGX](https://github.com/EpicGames/UnrealEngine/blob/4.27/Engine/Shaders/Private/ShadingModels.ush#L178)
		float3 specResult0 = Specular_TR(specular_color, roughtness0, NdotV, NdotL, NdotH, LdotH);
		float3 specResult1 = Specular_TR(specular_color, roughtness1, NdotV, NdotL, NdotH, LdotH);
		float3 specResult = lerp(specResult0, specResult1, specLobeBlend);
#endif

		// Darken spec where the *geometric* NdotL gets too low -
		// avoids it showing up on bumps in shadowed areas
		float edgeDarken = saturate(5.0 * dot(normalGeom, g_vecDirectionalLight));
		
		rgbDirectionalLightSpecular = (PI * g_rgbDirectionalLight) * (NdotL * edgeDarken * specResult * shadow);
	}

	// IBL spec - again two-lobe
	float3 rgbIBLSpecular;
	{
		float3 vecReflect = reflect(-vecCamera, normalShade);

		float gloss0 = gloss;
		float gloss1 = saturate(2.0 * gloss);

		float3 fresnelIBL0 = F_Schlick(specular_color, NdotV / (-3.0 * gloss0 + 4.0));
		float mipLevel0 = -9.0 * gloss0 + 9.0;
		float3 iblSpec0 = fresnelIBL0 * g_texCubeSpec.SampleLevel(g_ssTrilinearRepeat, vecReflect, mipLevel0);

		float3 fresnelIBL1 = F_Schlick(specular_color, NdotV / (-3.0 * gloss1 + 4.0));
		float mipLevel1 = -9.0 * gloss1 + 9.0;
		float3 iblSpec1 = fresnelIBL1 * g_texCubeSpec.SampleLevel(g_ssTrilinearRepeat, vecReflect, mipLevel1);

		rgbIBLSpecular = lerp(iblSpec0, iblSpec1, specLobeBlend);
	}

	float3 rgbLitSpecular = rgbDirectionalLightSpecular + rgbIBLSpecular;
	return rgbLitSpecular;
}

// Master lighting routine

void LightingMegashader(
	in Vertex i_vtx,
	in float3 i_vecCamera,
	in float4 i_uvzwShadow,
	in float3 rgbDiffuse,
	in float3 normalTangent,
	in float3 normalTangentBlurred,
	in float specReflectance,
	in float gloss,
	in float3 rgbDeepScatter,
	in GFSDK_FaceWorks_CBData faceworksData,
	out float3 o_rgbLit,
	uniform bool useNormalMap,
	uniform bool useSSS,
	uniform bool useDeepScatter)
{
	float3 normalGeom = normalize(i_vtx.m_normal);
	float3 vecCamera = normalize(i_vecCamera);
	float2 uv = i_vtx.m_uv;

	float3 normalShade, normalBlurred;
	if (useNormalMap)
	{
		// Transform normal maps to world space

		float3x3 matTangentToWorld = float3x3(
			normalize(i_vtx.m_tangent),
			normalize(cross(normalGeom, i_vtx.m_tangent)),
			normalGeom);

		normalShade = normalize(mul(normalTangent, matTangentToWorld));

		if (useSSS || useDeepScatter)
		{
			normalBlurred = normalize(mul(normalTangentBlurred, matTangentToWorld));
		}
	}
	else
	{
		normalShade = normalGeom;
		normalBlurred = normalGeom;
	}

	// Evaluate shadow map
	float shadow = EvaluateShadowVSM(i_uvzwShadow, normalGeom);

	float3 rgbLitDiffuse;
	if (useSSS)
	{
		// Evaluate diffuse lighting
		float3 rgbDiffuseLight = EvaluateSSSDiffuseLight(
			normalGeom, normalShade, normalBlurred,
			shadow, i_vtx.m_curvature, faceworksData);
		rgbLitDiffuse = rgbDiffuseLight * rgbDiffuse;

		// Remap shadow to 1/3-as-wide penumbra to match shadow from LUT.
		shadow = GFSDK_FaceWorks_SharpenShadow(shadow, g_shadowSharpening);
	}
	else
	{
		// Remap shadow to 1/3-as-wide penumbra to match shadow in SSS case.
		shadow = GFSDK_FaceWorks_SharpenShadow(shadow, g_shadowSharpening);

		// Evaluate diffuse lighting
		float3 rgbDiffuseLight = EvaluateDiffuseLight(normalGeom, normalShade, shadow);
		rgbLitDiffuse = rgbDiffuseLight * rgbDiffuse;
	}

	// Evaluate specular lighting
	float3 rgbLitSpecular = EvaluateSpecularLight(
		normalGeom, normalShade, vecCamera,
		specReflectance, gloss,
		shadow);

	// Put it all together
	o_rgbLit = rgbLitDiffuse + rgbLitSpecular;

	if (useDeepScatter)
	{
		float3 uvzShadow = i_uvzwShadow.xyz / i_uvzwShadow.w;

		// Apply normal offset to avoid silhouette edge artifacts
		// !!!UNDONE: move this to vertex shader
		float3 normalShadow = mul(normalGeom, g_matWorldToUvzShadowNormal);
		uvzShadow += normalShadow * g_deepScatterNormalOffset;

		float thickness = GFSDK_FaceWorks_EstimateThicknessFromParallelShadowPoisson32(
			faceworksData,
			g_texShadowMap, g_ssBilinearClamp, uvzShadow);

		float deepScatterFactor = GFSDK_FaceWorks_EvaluateDeepScatterDirectLight(
			faceworksData,
			normalBlurred, g_vecDirectionalLight, thickness);
		rgbDeepScatter *= g_deepScatterIntensity;
		o_rgbLit += (g_deepScatterIntensity * deepScatterFactor) * rgbDeepScatter *
					rgbDiffuse * g_rgbDirectionalLight;
	}

	// Apply tonemapping to the result
	o_rgbLit = Tonemap(o_rgbLit);
}

#endif // LIGHTING_HLSLI
