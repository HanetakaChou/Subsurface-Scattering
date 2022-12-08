/**
 * Copyright (C) 2012 Jorge Jimenez (jorge@iryoku.com)
 * Copyright (C) 2012 Diego Gutierrez (diegog@unizar.es)
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * 
 *    1. Redistributions of source code must retain the above copyright notice,
 *       this list of conditions and the following disclaimer.
 *
 *    2. Redistributions in binary form must reproduce the following disclaimer
 *       in the documentation and/or other materials provided with the 
 *       distribution:
 *
 *       "Uses Separable SSS. Copyright (C) 2012 by Jorge Jimenez and Diego
 *        Gutierrez."
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

//-----------------------------------------------------------------------------
// Configurable Defines

// Light diffusion should occur on the surface of the object, not in a screen
// oriented plane. Setting SSSS_FOLLOW_SURFACE to 1 will ensure that diffusion
// is more accurately calculated, at the expense of more memory accesses.
#ifndef SSSS_FOLLOW_SURFACE
#define SSSS_FOLLOW_SURFACE 0
#endif

//-----------------------------------------------------------------------------
// Porting Functions

#define SSSSLerp(a, b, t) lerp(a, b, t)
#define SSSSSaturate(a) saturate(a)
#define SSSS_UNROLL [unroll]

//-----------------------------------------------------------------------------
// Separable SSS Reflectance Pixel Shader

float4 SSSSBlurPS(
    float sssWidth,
    // The usual quad texture coordinates.
    float2 texcoord,
    // Direction of the blur:
    // First pass:   float2(1.0, 0.0)
    // Second pass:  float2(0.0, 1.0)
    float2 dir)
{

    // Fetch color of current pixel:
    float4 colorM = SSSS_COLOR_SOURCE_POINT(texcoord);

    // Fetch z component of view space position of current pixel:
    float viewPositionZM = SSSS_VIEWPOSITIONZ_SOURCE_POINT(texcoord);

    // The deviations in world space (in mm) is fixed and has been stored in the w component of the kernel.
    // The shader code is merely to transform the deviations from the world space to the texture space.
    // 0.001f : from mm to m
    // projectionY : the 2 row and 2 column of the projection matrix
    // 0.5f : from NDC to texture space
    float metersPerWorldUnit = 3.0 * 0.001 * 0.5 / sssWidth;
    float projectionY = SSSS_PROJECTIONY_SOURCE;
    float scale = 0.5 * projectionY / viewPositionZM * 0.001 / metersPerWorldUnit;

    // Calculate the final step to fetch the surrounding pixels:
    float2 finalStep = scale * dir;

    // Modulate it using the alpha of the albedo.
    // finalStep *= SSSS_STREGTH_SOURCE(texcoord);

    // Accumulate the center sample:
    float4 colorBlurred = colorM;
    colorBlurred.rgb *= SSSS_KERNEL_SOURCE(0).rgb;

    // Accumulate the other samples:
    SSSS_UNROLL
    for (int i = 1; i < SSSS_N_SAMPLES; i++)
    {
        float4 tempKernel = SSSS_KERNEL_SOURCE(i);
        float3 preIntergatedKernel = tempKernel.xyz;
        float offsetWorldInMM = tempKernel.w;

        // Fetch color and depth for current sample:
        float2 offset = texcoord + offsetWorldInMM * finalStep;
        float4 color = SSSS_COLOR_SOURCE(offset);

#if SSSS_FOLLOW_SURFACE == 1
        // If the difference in depth is huge, we lerp color back to "colorM":
        float viewPositionZ = SSSS_VIEWPOSITIONZ_SOURCE(offset);
        float s = SSSSSaturate(450.0 * 0.001 / metersPerWorldUnit * projectionY * abs(viewPositionZM - viewPositionZ));
        color.rgb = SSSSLerp(color.rgb, colorM.rgb, s);
#endif

        // Accumulate:
        colorBlurred.rgb += preIntergatedKernel * color.rgb;
    }

    return colorBlurred;
}
