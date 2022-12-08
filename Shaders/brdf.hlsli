//
// Copyright (C) YuqiaoZhang(HanetakaChou)
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published
// by the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.
//

#ifndef _BRDF_HLSLI_
#define _BRDF_HLSLI_ 1

#include "math_consts.hlsli"

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

float3 Diffuse_Disney(float3 diffuse_color, float roughness, float NdotV, float NdotL, float LdotH)
{
	// UE: [Diffuse_Burley](https://github.com/EpicGames/UnrealEngine/blob/4.27/Engine/Shaders/Private/BRDF.ush#L253)
	// U3D: [DisneyDiffuse](https://github.com/Unity-Technologies/Graphics/blob/v10.8.0/com.unity.render-pipelines.core/ShaderLibrary/BSDF.hlsl#L414)

	float f90 = 0.5 + 2.0 * LdotH * LdotH * roughness;

	float3 term_v = F_Schlick(float3(1.0, 1.0, 1.0), f90, NdotV);
	float3 term_L = F_Schlick(float3(1.0, 1.0, 1.0), f90, NdotL);

	return (1.0 / PI) * diffuse_color * term_v * term_L;
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

float3 Dual_Specular_TR(float material_roughness_0, float material_roughness_1, float material_lobe_mix, float3 specular_color, float roughness, float subsurface_mask, float NdotV, float NdotL, float NdotH, float LdotH)
{
	float material_roughness_average = lerp(material_roughness_0, material_roughness_1, material_lobe_mix);
	float average_to_roughness_0 = material_roughness_0 / material_roughness_average;
	float average_to_roughness_1 = material_roughness_1 / material_roughness_average;

	float surface_roughness_average = roughness;
	float surface_roughness_0 = clamp(average_to_roughness_0 * surface_roughness_average, 0.02, 1.0);
	float surface_roughness_1 = clamp(average_to_roughness_1 * surface_roughness_average, 0.0, 1.0);

	// UE4: SubsurfaceProfileBxDF
	surface_roughness_0 = lerp(1.0, surface_roughness_0, clamp(10.0 * subsurface_mask, 0.0, 1.0));
	surface_roughness_1 = lerp(1.0, surface_roughness_1, clamp(10.0 * subsurface_mask, 0.0, 1.0));

	float3 radiance_specular_0 = Specular_TR(specular_color, surface_roughness_0, NdotV, NdotL, NdotH, LdotH);
	float3 radiance_specular_1 = Specular_TR(specular_color, surface_roughness_1, NdotV, NdotL, NdotH, LdotH);
	float3 radiance_specular = lerp(radiance_specular_0, radiance_specular_1, material_lobe_mix);
	return radiance_specular;
}

#endif