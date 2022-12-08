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

#ifndef _SUBSURFACE_SCATTERING_TEXTURING_MODE_HLSLI_
#define _SUBSURFACE_SCATTERING_TEXTURING_MODE_HLSLI_ 1

float3 subsurface_scattering_total_diffuse_reflectance_pre_scatter_from_albedo(bool is_post_scatter_texturing_mode, float3 albedo)
{
	[branch]
	if (!is_post_scatter_texturing_mode)
	{
		return sqrt(albedo);
	}
	else
	{
		return 1.0F;
	}
}

float3 subsurface_scattering_total_diffuse_reflectance_post_scatter_from_albedo(bool is_post_scatter_texturing_mode, float3 albedo)
{
	[branch]
	if (!is_post_scatter_texturing_mode)
	{
		return sqrt(albedo);
	}
	else
	{
		return albedo;
	}
}

#endif