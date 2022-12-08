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

cbuffer UpdatedPerFrame : register(b0)
{
    row_major float4x4 currProj;
    
    /**
    * Filter kernel layout is as follows:
    *   - Weights in the RGB channels.
    *   - Offsets in the A channel.
    */
    float4 kernel[33];

    /**
    * See |SeparableSSS.hlsli| for more details.
    */
    float sssWidth;

    /**
    * See |SeparableSSS.hlsli| for more details.
    */
    int nSamples;
}

// Texture used to fecth the SSS strength:
Texture2D strengthTex : register(t0);

/**
        * This is a SRGB or HDR color input buffer, which should be the final
        * color frame, resolved in case of using multisampling. The desired
        * SSS strength should be stored in the alpha channel (1 for full
        * strength, 0 for disabling SSS). If this is not possible, you an
        * customize the source of this value using SSSS_STREGTH_SOURCE.
        *
        * When using non-SRGB buffers, you
        * should convert to linear before processing, and back again to gamma
        * space before storing the pixels (see Chapter 24 of GPU Gems 3 for
        * more info)
        *
        * IMPORTANT: WORKING IN A NON-LINEAR SPACE WILL TOTALLY RUIN SSS!
        */
Texture2D colorTex : register(t1);

/**
 * The linear depth buffer of the scene, resolved in case of using
 * multisampling. The resolve should be a simple average to avoid
 * artifacts in the silhouette of objects.
 */
Texture2D depthTex : register(t2);

SamplerState LinearSampler : register(s0);

SamplerState PointSampler : register(s1);

#define SSSS_FOVY 20

#define SSSS_FOLLOW_SURFACE 1

#define SSSS_STREGTH_SOURCE(coord) (strengthTex.SampleLevel(LinearSampler, coord, 0).a)

#define SSSS_COLOR_SOURCE_POINT(coord) (colorTex.SampleLevel(PointSampler, coord, 0))
#define SSSS_COLOR_SOURCE(coord) (colorTex.SampleLevel(LinearSampler, coord, 0))

#define SSSS_VIEWPOSITIONZ_SOURCE_POINT(coord) (depthTex.SampleLevel(PointSampler, coord, 0).r)
#define SSSS_VIEWPOSITIONZ_SOURCE(coord) (depthTex.SampleLevel(LinearSampler, coord, 0).r)

#define SSSS_N_SAMPLES nSamples
#define SSSS_KERNEL_SOURCE(sampleindex) (kernel[sampleindex])

#define SSSS_PROJECTIONY_SOURCE currProj[1][1]

// And include our header!
#include "../SeparableSSS.hlsli"

/**
 * Function wrappers
 */
void DX10_SSSSBlurVS(float4 position : POSITION,
    out float4 svposition : SV_POSITION,
    inout float2 texcoord : TEXCOORD0) 
{
    svposition = position;
}

float4 DX10_SSSSBlurXPS(float4 position : SV_POSITION, float2 texcoord : TEXCOORD0) : SV_TARGET
{
    return SSSSBlurPS(sssWidth, texcoord, float2(1.0, 0.0));
}

float4 DX10_SSSSBlurYPS(float4 position : SV_POSITION, float2 texcoord : TEXCOORD0) : SV_TARGET
{
    return SSSSBlurPS(sssWidth, texcoord, float2(0.0, 1.0));
}