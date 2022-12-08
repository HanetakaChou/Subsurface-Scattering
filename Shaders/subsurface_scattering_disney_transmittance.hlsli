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

#ifndef _SUBSURFACE_SCATTERING_DISNEY_TRANSMITTANCE_HLSLI_
#define _SUBSURFACE_SCATTERING_DISNEY_TRANSMITTANCE_HLSLI_ 1

#define LOG2_E 1.44269504088896340736

// Computes the fraction of light passing through the object.
// Evaluate Int{0, inf}{2 * Pi * r * R(sqrt(r^2 + d^2))}, where R is the diffusion profile.
// Ref: Approximate Reflectance Profiles for Efficient Subsurface Scattering by Pixar (BSSRDF only).
float3 EvaluateTransmittance(float3 S, float thickness)
{
    // The transmittance coefficient T(d) is precalculated based on diffusion profile.

    // Thickness and SSS mask are decoupled for artists.
    // In theory, we should modify the thickness by the inverse of the mask scale of the profile.
    // thickness /= subsurfaceMask;

    float3 exp_13 = exp2(((LOG2_E * (-1.0 / 3.0)) * thickness) * S); // Exp[-S * t / 3]

    // T = (1/4 * A) * (e^(-S * t) + 3 * e^(-S * t / 3))
    // the 'A' is multiplied in the 'full screen sss blur pass'
    return 0.25 * (exp_13 * (exp_13 * exp_13 + 3));
}

float EvaluateTransmittanceLightAttenuation(float3 N, float3 L)
{
    // Using the transmittance coefficient T(d), we finally approximate the transmitted lighting from the back of the object
    
    float NoL = saturate(0.3 + dot(-N, L));
    return NoL;
}

#endif