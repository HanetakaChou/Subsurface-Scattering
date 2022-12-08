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


cbuffer UpdatedPerFrame : register(b0)
{
	row_major float4x4 view;
	row_major float4x4 projection;
	row_major float4x4 world;
	float intensity;
}

TextureCube skyTex : register(t0);

SamplerState LinearSampler : register(s0)
{
	Filter = MIN_MAG_MIP_LINEAR;
	AddressU = Clamp;
	AddressV = Clamp;
};

void SkyDomeVS(float4 position : POSITION,
	out float4 svposition : SV_POSITION,
	out float3 texcoord : TEXCOORD0) {
	float4x4 worldViewProjection = mul(mul(world, view), projection);
	svposition = mul(position, worldViewProjection);
	texcoord = position.xyz;
}

float4 SkyDomePS(float4 position : SV_POSITION,
	float3 texcoord : TEXCOORD0) : SV_TARGET0{
return float4(intensity * skyTex.Sample(LinearSampler, texcoord).rgb, 0.0);
}


DepthStencilState EnableDepthDisableStencil {
	DepthEnable = TRUE;
	StencilEnable = FALSE;
};

BlendState NoBlending {
	AlphaToCoverageEnable = FALSE;
	BlendEnable[0] = FALSE;
};


technique10 RenderSkyDome {
	pass RenderSkyDome {
		//SetVertexShader(CompileShader(vs_4_0, SkyDomeVS(mul(mul(world, view), projection))));
		//SetGeometryShader(NULL);
		//SetPixelShader(CompileShader(ps_4_0, SkyDomePS()));

		SetDepthStencilState(EnableDepthDisableStencil, 0);
		SetBlendState(NoBlending, float4(0.0f, 0.0f, 0.0f, 0.0f), 0xFFFFFFFF);
	}
}
