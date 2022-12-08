cbuffer UpdatedPerFrame : register(b0)
{
	row_major float4x4 currProj;
	float3 scatteringDistance;
	float padding_scatteringDistance;
	float worldScale;
	float postscatterEnabled;
	int sampleBudget;
	int pixelsPerSample;
}

Texture2D g_albedo_texture : register(t0);

// NOTE: we should output zero for non-skin material
Texture2D g_total_diffuse_reflectance_pre_scatter_multiply_form_factor_texture : register(t1);

Texture2D depthTex : register(t2);

SamplerState PointSampler : register(s1);

#include "../subsurface_scattering_texturing_mode.hlsli"

inline float3 SSS_TOTAL_DIFFUSE_REFLECTANCE_PRE_SCATTER_MULTIPLY_FORM_FACTOR_SOURCE(float2 pixelCoord)
{
	// NOTE: use SampleLevel rather than Sample
	// warning X3595: gradient instruction used in a loop with varying iteration; partial derivatives may have undefined value
	float3 total_diffuse_reflectance_pre_scatter_multiply_form_factor = g_total_diffuse_reflectance_pre_scatter_multiply_form_factor_texture.SampleLevel(PointSampler, pixelCoord, 0).rgb;
	return total_diffuse_reflectance_pre_scatter_multiply_form_factor;
}

inline float3 SSS_TOTAL_DIFFUSE_REFLECTANCE_POST_SCATTER_SOURCE(float2 pixelCoord)
{
	float3 albedo = g_albedo_texture.SampleLevel(PointSampler, pixelCoord, 0).rgb;
	float3 total_diffuse_reflectance_post_scatter = subsurface_scattering_total_diffuse_reflectance_post_scatter_from_albedo((postscatterEnabled > 0.0), albedo);
	return total_diffuse_reflectance_post_scatter;
}

inline float SSS_SUBSURFACE_MASK_SOURCE(float2 pixelCoord)
{
	return g_albedo_texture.SampleLevel(PointSampler, pixelCoord, 0).a;
}

inline float SSS_PROJECTION_X_SOURCE()
{
	return currProj[0][0];
}

inline float SSS_PROJECTION_Y_SOURCE()
{
	return currProj[1][1];
}

inline float2 uv_to_ndcxy(float2 uv)
{
	return uv * float2(2.0, -2.0) + float2(-1.0, 1.0);
}

inline float ndcz_to_viewpositionz(float ndcz, float4x4 proj)
{
	return proj[3][2] / (ndcz - proj[2][2]);
}

inline float3 ndc_to_view(float3 ndc, float4x4 proj)
{
	float view_position_z = ndcz_to_viewpositionz(ndc.z, proj);
	float2 view_position_xy = ndc.xy * view_position_z / float2(proj[0][0], proj[1][1]);
	return float3(view_position_xy, view_position_z);
}

inline float SSS_VIEW_SPACE_POSITION_Z_SOURCE(float2 pixelCoord)
{
	// NOTE: use SampleLevel rather than Sample
	// warning X3595: gradient instruction used in a loop with varying iteration; partial derivatives may have undefined value
	float depth = depthTex.SampleLevel(PointSampler, pixelCoord, 0).r;
	float view_position_z = ndcz_to_viewpositionz(depth, currProj);
	return view_position_z;
}

inline float3 SSS_VIEW_SPACE_POSITION_SOURCE(float2 pixelCoord)
{
	float depth = depthTex.SampleLevel(PointSampler, pixelCoord, 0).r;
	float3 view_position = ndc_to_view(float3(uv_to_ndcxy(pixelCoord), depth), currProj);
	return view_position;
}

inline float2 SSS_PIXELS_PER_UV()
{
	float outWidth;
	float outHeight;
	float outNumberOfLevels;
	g_total_diffuse_reflectance_pre_scatter_multiply_form_factor_texture.GetDimensions(0, outWidth, outHeight, outNumberOfLevels);
	return float2(outWidth, outHeight);
}

#include "../subsurface_scattering_disney_blur.hlsli"

void SSS_Blur_VS(float4 position : POSITION, out float4 svposition : SV_POSITION, inout float2 texcoord : TEXCOORD0)
{
	svposition = position;
}

float4 SSS_Blur_PS(float4 position: SV_POSITION, float2 texcoord : TEXCOORD0) : SV_TARGET
{
	float3 color = subsurface_scattering_disney_blur(scatteringDistance, worldScale, pixelsPerSample, sampleBudget, texcoord);
	return float4(color, 1.0);
}
