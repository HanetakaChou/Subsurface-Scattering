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

#ifndef _SKYDOME_H_
#define _SKYDOME_H_ 1

#include <sdkddkver.h>
#define NOMINMAX 1
#define WIN32_LEAN_AND_MEAN 1
#include <DXUT.h>
#include <SDKmisc.h>
#include <SDKmesh.h>
#include <DirectXMath.h>
#include "RenderTarget.h"
#include <string>

class SkyDome
{
public:
	SkyDome(ID3D11Device* device, const std::wstring& dir, float intensity = 1.0f);
	~SkyDome();

	void render(ID3D11DeviceContext* context, const DirectX::XMFLOAT4X4& view, const DirectX::XMFLOAT4X4& projection, float scale);

	void setIntensity(float var_intensity) { this->intensity = var_intensity; }
	float getIntensity() const { return intensity; }

	void setAngle(const DirectX::XMFLOAT2& var_angle) { this->angle = var_angle; }
	DirectX::XMFLOAT2 getAngle() const { return angle; }

private:
	void createMesh(ID3D11Device* device, const std::wstring& dir);

	ID3D11VertexShader* SkyDomeVS;
	ID3D11PixelShader* SkyDomePS;
	ID3D11Buffer* CbufUpdatedPerFrame;
	ID3D11DepthStencilState* EnableDepthDisableStencil;
	ID3D11BlendState* NoBlending;
	ID3D11SamplerState* LinearSampler;

	CDXUTSDKMesh mesh;
	float intensity;
	DirectX::XMFLOAT2 angle;
};

#endif
