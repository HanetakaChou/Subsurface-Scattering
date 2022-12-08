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

#ifndef _SSSBlur_H_
#define _SSSBlur_H_ 1

#include <sdkddkver.h>
#define NOMINMAX 1
#define WIN32_LEAN_AND_MEAN 1
#include <DXUT.h>
#include <DirectXMath.h>
#include "RenderTarget.h"
#include <string>

class SSSBlur
{
public:
	SSSBlur(ID3D11Device* device,
		float worldScale,
		bool postscatterEnabled,
		int sampleBudget,
		int pixelsPerSample);
	~SSSBlur();

	void go(ID3D11DeviceContext* context,
		ID3D11RenderTargetView* mainRTV,
		ID3D11ShaderResourceView* irradianceSRV,
		ID3D11ShaderResourceView* depthSRV,
		ID3D11DepthStencilView* depthDSV,
		ID3D11ShaderResourceView* albedoSRV);

	void setWorldScale(float worldScale)
	{
		this->m_worldScale = std::max(0.001f, worldScale);
	}

	void setPostScatterEnabled(bool postscatterEnabled)
	{
		this->m_postscatterEnabled = postscatterEnabled;
	}

	void setStrength(DirectX::XMFLOAT3 scatteringDistance)
	{
		this->m_scatteringDistance = scatteringDistance;
	}

	void setNSamples(int sampleBudget)
	{
		this->m_sampleBudget = std::max(1, sampleBudget);
	}

	void setPixelsPerSample(int pixelsPerSample)
	{
		this->m_pixelsPerSample = std::max(4, pixelsPerSample);
	}

private:
	DirectX::XMFLOAT3 m_scatteringDistance;
	float m_worldScale;
	bool m_postscatterEnabled;
	int m_sampleBudget;
	int m_pixelsPerSample;

	ID3D11VertexShader* SSS_VS;
	ID3D11PixelShader* SSS_Blur_PS;
	ID3D11Buffer* CbufUpdatedPerFrame;
	ID3D11DepthStencilState* BlurStencil;
	ID3D11BlendState* AddBlending;
	ID3D11SamplerState* PointSampler;
	ID3D11SamplerState* LinearSampler;
	RenderTarget* tmpRT;
	Quad* quad;
};

#endif
