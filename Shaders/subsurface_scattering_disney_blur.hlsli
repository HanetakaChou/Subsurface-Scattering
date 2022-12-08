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

// Note: Provided by the User!
// 
// SSS_ALBEDO_SOURCE_POINT
// 
// "Combining Pre-Scatter and Post-Scatter Texturing" of [Eugene dEon, David Luebke. "Advanced Techniques for Realistic Real-Time Skin Rendering." GPU Gem 3.](https://developer.nvidia.com/gpugems/gpugems3/part-iii-rendering/chapter-14-advanced-techniques-realistic-real-time-skin)
// Pre- and Post-Scatter Texturing Mode - sqrt(albedo)
// 
// "Post-Scatter Texturing" of [Eugene dEon, David Luebke. "Advanced Techniques for Realistic Real-Time Skin Rendering." GPU Gem 3.](https://developer.nvidia.com/gpugems/gpugems3/part-iii-rendering/chapter-14-advanced-techniques-realistic-real-time-skin)
// Post-Scatter Texturing Mode - albedo
//

#ifndef _SUBSURFACE_SCATTERING_DISNEY_BLUR_HLSLI_
#define _SUBSURFACE_SCATTERING_DISNEY_BLUR_HLSLI_ 1

#include "math_consts.hlsli"
#include "low_discrepancy_sequence.hlsli"

#define SSS_MIN_PIXELS_PER_SAMPLE 4
#define SSS_MAX_SAMPLE_BUDGET 80

// https://zero-radiance.github.io/post/sampling-diffusion/
// S = ShapeParam = 1 / ScatteringDistance = 1 / d
// d = ScatteringDistance = 1 / ShapeParam = 1 / S
// CDF is the random number (the value of the CDF): [0, 1).
// r is the sampled radial distance, s.t. (CDF = 0 -> r = 0) and (CDF = 1 -> r = Inf).
// PDF is the derivative of the (marginal) CDF. Actually, '2 * PI' should be multiplied to convert the normalized diffusion profile to 1D, namely, PDF = (DiffusionProfile * r / A) * (2 * PI).
float diffusion_profile_sample_r(float d, float cdf);
float diffusion_profile_evaluate_cdf(float d, float r);
float diffusion_profile_evaluate_rcp_pdf(float d, float r);
float3 diffusion_profile_evaluate_pdf(float3 S, float r);

float3 subsurface_scattering_disney_blur(const float3 scattering_distance, const float world_scale, const int pixels_per_sample, const int sample_budget, const float2 center_uv)
{
	const float dist_scale = SSS_SUBSURFACE_MASK_SOURCE(center_uv);
	// Early Out
	[branch]
	if (dist_scale < (1.0 / 255.0))
	{
		float3 total_diffuse_reflectance_pre_scatter_multiply_form_factor = SSS_TOTAL_DIFFUSE_REFLECTANCE_PRE_SCATTER_MULTIPLY_FORM_FACTOR_SOURCE(center_uv);

		float3 total_diffuse_reflectance_post_scatter = SSS_TOTAL_DIFFUSE_REFLECTANCE_POST_SCATTER_SOURCE(center_uv);
		
		float3 radiance = total_diffuse_reflectance_post_scatter * total_diffuse_reflectance_pre_scatter_multiply_form_factor;
		return radiance;
	}

	// In UE4, the "dist_scale" is applied to the "scattering_distance" rather than the "uv_per_mm".  
	// This is equivalent since the "r" evaluated by the "diffusion_profile_sample_r" is proportional to the "scattering_distance".
	const float meters_per_unit = world_scale;
#if 0
	// Unity3D
	float3 center_view_space_position = SSS_VIEW_SPACE_POSITION_SOURCE(center_uv);
	const float center_view_space_position_z = center_view_space_position.z;
	float3 corner_view_space_position = SSS_VIEW_SPACE_POSITION_SOURCE(center_uv + 0.5 * (float2(1.0, 1.0) / SSS_PIXELS_PER_UV()));
	float2 units_per_pixel = max(float2(0.0001, 0.0001), 2.0 * abs(float2(corner_view_space_position.x - center_view_space_position.x, corner_view_space_position.y - center_view_space_position.y)));
	const float mms_per_unit = 1000.0 * meters_per_unit * (1.0 / dist_scale);
	const float2 pixels_per_mm = (1.0 / units_per_pixel) * (1.0 / mms_per_unit);
	const float2 uv_per_mm = pixels_per_mm * (float2(1.0, 1.0) / SSS_PIXELS_PER_UV());
#else
	// UE4
	const float center_view_space_position_z = SSS_VIEW_SPACE_POSITION_Z_SOURCE(center_uv);
	const float mms_per_unit = 1000.0 * meters_per_unit * (1.0 / dist_scale);
	const float2 uv_per_mm = 0.5 * float2(SSS_PROJECTION_X_SOURCE(), SSS_PROJECTION_Y_SOURCE()) * (1.0 / center_view_space_position_z) * (1.0 / mms_per_unit);
	const float2 pixels_per_mm = SSS_PIXELS_PER_UV() * uv_per_mm;
#endif

	// Unity3D <=> UE4
	// ScatteringDistance = MeanFreePathColor * MeanFreePathDistance / GetScalingFactor(SurfaceAlbedo)
	const float3 S = float3(1.0, 1.0, 1.0) / scattering_distance.rgb;
	const float d = max(max(scattering_distance.r, scattering_distance.g), scattering_distance.b);

	// Center Sample Reweighting
	//
	// The original burley algorithm involes monte car sampling. Given a random variable [0,1],
	// find the distance of that point from the center using the CDF, and then divide by PDF.
	// But it is somewhat inefficient because it is weighted heavily towards the center.
	//
	// Instead, we are going to split the [0,1] random variable range. First, we figure out the
	// radius (R) of the center sample in world space. Second, we are going to determine the random
	// variable (T) such that CDF(R) = T. Then we split the range into two segments.
	//
	// 1. The center sample, which include the random variable values from [0,T].
	// 2. All other samples, which include the random variable values from [T,1].
	//
	// With the center sample is scaled the weight T and the rest of the samples are weighted
	// by (1-T). There shouldn't be any bias, except for small errors due to precision.
	const float center_sample_radius_in_mm = 0.5 * (1.0 / pixels_per_mm.x + 1.0 / pixels_per_mm.y);
	const float center_sample_cdf = diffusion_profile_evaluate_cdf(d, center_sample_radius_in_mm);

	float3 sum_numerator = float3(0.0, 0.0, 0.0);
	float3 sum_denominator = float3(0.0, 0.0, 0.0);

	// Filter radius is, strictly speaking, infinite.
	// The magnitude of the function decays exponentially, but it is never truly zero.
	// To estimate the radius, we can use adapt the "three-sigma rule" by defining
	// the radius of the kernel by the value of the CDF which corresponds to 99.7%
	// of the energy of the filter.
	const float filter_radius = diffusion_profile_sample_r(d, 0.997);
	float sample_count = int(float(PI) * (filter_radius * pixels_per_mm.x) * (filter_radius * pixels_per_mm.y) * (1.0 / float(max(pixels_per_sample, int(SSS_MIN_PIXELS_PER_SAMPLE)))));
	sample_count = min(sample_count, min(sample_budget, int(SSS_MAX_SAMPLE_BUDGET)));

	[loop]
	for (int sample_index = 0; sample_index < int(SSS_MAX_SAMPLE_BUDGET) && sample_index < sample_count; ++sample_index)
	{
		float2 xi = hammersley_2d(sample_index, sample_count);

		// Center Sample Reweighting
		xi.x = lerp(center_sample_cdf, 1.0, xi.x);

		// Sampling Diffusion Profile
		float r = diffusion_profile_sample_r(d, xi.x);
		float theta = 2.0 * float(PI) * xi.y;

		// Bilateral Filter
		float2 sample_uv = center_uv + uv_per_mm * float2(cos(theta), sin(theta)) * r;

		// The "sample_form_factor" may be zero even if the "sample_dist_scale" is NOT zero
		float sample_dist_scale = SSS_SUBSURFACE_MASK_SOURCE(sample_uv);

		[branch]
		if (sample_dist_scale >= (1.0 / 255.0))
		{
			float3 sample_total_diffuse_reflectance_pre_scatter_multiply_form_factor = SSS_TOTAL_DIFFUSE_REFLECTANCE_PRE_SCATTER_MULTIPLY_FORM_FACTOR_SOURCE(sample_uv);

			float rcp_pdf = diffusion_profile_evaluate_rcp_pdf(d, r);

			// Bilateral Filter
			float sample_view_space_position_z = SSS_VIEW_SPACE_POSITION_Z_SOURCE(sample_uv);
			float relative_position_z_mm = mms_per_unit * (sample_view_space_position_z - center_view_space_position_z);
			float r_bilateral_weight = sqrt(r * r + relative_position_z_mm * relative_position_z_mm);

			float3 pdf = diffusion_profile_evaluate_pdf(S, r_bilateral_weight);

			// normalized_diffusion_profile = pdf / r
			// (1.0 / float(N)) * total_diffuse_reflectance * (pdf / r) * form_factor * r * rcp_pdf 
			// = (1.0 / float(N)) * total_diffuse_reflectance * pdf * form_factor * rcp_pdf
			// = (1.0 / float(N)) * total_diffuse_reflectance_post_scatter * pdf * (total_diffuse_reflectance_pre_scatter * form_factor) * rcp_pdf
			// the 'total_diffuse_reflectance_post_scatter' will be calculated later while the 'total_diffuse_reflectance_pre_scatter * form_factor' is calculated here 
			float3 sample_numerator = pdf * sample_total_diffuse_reflectance_pre_scatter_multiply_form_factor * rcp_pdf;

			// (1.0 / float(N)) * pdf * rcp_pdf
			float3 sample_denominator = pdf * rcp_pdf;

			sum_numerator += sample_numerator;

			// assumed to be N since the pdf is normalized and the common divisor '1.0 / float(N)' is reduced
			sum_denominator += sample_denominator;
		}
	}

	// Center Sample Reweighting
	float3 sum_total_diffuse_reflectance_pre_scatter_multiply_form_factor = sum_numerator / max(sum_denominator, float3(FLT_MIN, FLT_MIN, FLT_MIN));
	float3 center_total_diffuse_reflectance_pre_scatter_multiply_form_factor = SSS_TOTAL_DIFFUSE_REFLECTANCE_PRE_SCATTER_MULTIPLY_FORM_FACTOR_SOURCE(center_uv);
	float3 total_diffuse_reflectance_pre_scatter_multiply_form_factor = lerp(sum_total_diffuse_reflectance_pre_scatter_multiply_form_factor, center_total_diffuse_reflectance_pre_scatter_multiply_form_factor, center_sample_cdf);

	float3 total_diffuse_reflectance_post_scatter = SSS_TOTAL_DIFFUSE_REFLECTANCE_POST_SCATTER_SOURCE(center_uv);
	
	float3 radiance = total_diffuse_reflectance_post_scatter * total_diffuse_reflectance_pre_scatter_multiply_form_factor;
	return radiance;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
//
//    IMPLEMENTATION
//
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

#define LOG2_E 1.44269504088896340736

float diffusion_profile_sample_r(float d, float cdf)
{
	float u = 1 - cdf; // Convert CDF to CCDF

	float g = 1 + (4 * u) * (2 * u + sqrt(1 + (4 * u) * u));

	// g^(-1/3)
	float n = exp2(log2(g) * (-1.0 / 3.0));
	// g^(+1/3)
	float p = (g * n) * n;
	// 1 + g^(+1/3) + g^(-1/3)
	float c = 1 + p + n;
	// 3 * Log[4 * u]
	float b = (3 / LOG2_E * 2) + (3 / LOG2_E) * log2(u);
	// 3 * Log[c / (4 * u)]
	float x = (3 / LOG2_E) * log2(c) - b;

	// x = S * r
	// r = x * rcpS = x * d
	float r = x * d;
	return r;
}

float diffusion_profile_evaluate_cdf(float d, float r)
{
	// x = S * r = (1.0 / d) * r
	// exp_13 = Exp[-x/3]
	float exp_13 = exp2(((LOG2_E * (-1.0 / 3.0)) * r) * (1.0 / d));
	// exp_1  = Exp[-x] = exp_13 * exp_13 * exp_13
	// exp_sum = -0.25 * exp_1 - 0.75 * exp_13 =  exp_13 * (-0.75 - 0.25 * exp_13 * exp_13)
	float exp_sum = exp_13 * (-0.75 - 0.25 * exp_13 * exp_13);
	// 1 - 0.75 * Exp[-S * r / 3]  - 0.25 * Exp[-S * r])
	float cdf = 1.0 + exp_sum;
	return cdf;
}

float diffusion_profile_evaluate_rcp_pdf(float d, float r)
{
	// x = S * r = (1.0 / d) * r
	// exp_13 = Exp[-x/3]
	float exp_13 = exp2(((LOG2_E * (-1.0 / 3.0)) * r) * (1.0 / d));
	// exp_1  = Exp[-x] = exp_13 * exp_13 * exp_13
	// exp_sum = exp_1 + exp_13 =  exp_13 * (1 + exp_13 * exp_13)
	float exp_sum = exp_13 * (1 + exp_13 * exp_13);
	// rcpExp = (1.0 / exp_sum)
	float rcpExp = (1.0 / exp_sum);

	// rcpS = d
	// (8 * PI) / S / (Exp[-S * r / 3] + Exp[-S * r])
	float rcp_pdf = (8.0 * PI) * d * rcpExp;
	return rcp_pdf;
}

float3 diffusion_profile_evaluate_pdf(float3 S, float r)
{
	// Exp[-s * r / 3]
	float3 exp_13 = exp2(((LOG2_E * (-1.0 / 3.0)) * r) * S);
	// Exp[-s * r / 3] + Exp[-S * r]
	float3 exp_sum = exp_13 * (1 + exp_13 * exp_13);
	// S / (8 * PI) * (Exp[-S * r / 3] + Exp[-S * r])
	float3 pdf = S / (8.0 * PI) * exp_sum;
	return pdf;
}

#endif