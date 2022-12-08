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

#include <sstream>
#include "SSSBlur.h"
#include "Demo.h"

#include "../../dxbc/SSS_Blur_VS_bytecode.inl"
#include "../../dxbc/SSS_Blur_PS_bytecode.inl"

struct UpdatedPerFrame
{
	__declspec(align(16)) DirectX::XMFLOAT4X4 currProj;
	__declspec(align(16)) DirectX::XMFLOAT3 scatteringDistance;
	float padding_scatteringDistance;
	float worldScale;
	float postscatterEnabled;
	int sampleBudget;
	int pixelsPerSample;
};

#define CB_UPDATEDPERFRAME 0
#define TEX_ALBEDO 0
#define TEX_IRRADIANCE 1
#define TEX_DEPTH 2
#define SAMP_POINT 0
#define SAMP_LINEAR 1

using namespace std;

SSSBlur::SSSBlur(ID3D11Device* device,
	float worldScale,
	bool postscatterEnabled,
	int sampleBudget,
	int pixelsPerSample) : m_worldScale(std::max(0.001f, worldScale)),
	m_postscatterEnabled(postscatterEnabled),
	m_sampleBudget(std::max(1, sampleBudget)),
	m_pixelsPerSample(std::max(4, pixelsPerSample))
{
	HRESULT hr;

	D3D11_BUFFER_DESC UpdatedPerFrameDesc =
	{
		sizeof(struct UpdatedPerFrame),
		D3D11_USAGE_DYNAMIC,
		D3D11_BIND_CONSTANT_BUFFER,
		D3D11_CPU_ACCESS_WRITE,
	};
	V(device->CreateBuffer(&UpdatedPerFrameDesc, NULL, &CbufUpdatedPerFrame));

	V(device->CreateVertexShader(SSS_Blur_VS_bytecode, sizeof(SSS_Blur_VS_bytecode), NULL, &SSS_VS));
	V(device->CreatePixelShader(SSS_Blur_PS_bytecode, sizeof(SSS_Blur_PS_bytecode), NULL, &SSS_Blur_PS));

	D3D11_DEPTH_STENCIL_DESC BlurStencilDesc = {};
	BlurStencilDesc.DepthEnable = TRUE;
	BlurStencilDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
	BlurStencilDesc.DepthFunc = D3D11_COMPARISON_LESS;
	BlurStencilDesc.StencilEnable = FALSE;
	BlurStencilDesc.StencilReadMask = D3D11_DEFAULT_STENCIL_READ_MASK;
	BlurStencilDesc.StencilWriteMask = D3D11_DEFAULT_STENCIL_WRITE_MASK;
	BlurStencilDesc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
	BlurStencilDesc.BackFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
	BlurStencilDesc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
	BlurStencilDesc.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
	BlurStencilDesc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
	BlurStencilDesc.BackFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
	BlurStencilDesc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
	BlurStencilDesc.BackFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
	BlurStencilDesc.DepthEnable = FALSE;
	BlurStencilDesc.StencilEnable = TRUE;
	BlurStencilDesc.FrontFace.StencilFunc = D3D11_COMPARISON_EQUAL;
	V(device->CreateDepthStencilState(&BlurStencilDesc, &BlurStencil));

	D3D11_BLEND_DESC AddBlendingDesc = {};
	AddBlendingDesc.AlphaToCoverageEnable = FALSE;
	AddBlendingDesc.IndependentBlendEnable = FALSE;
	AddBlendingDesc.RenderTarget[0].BlendEnable = TRUE;
	AddBlendingDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_ONE;
	AddBlendingDesc.RenderTarget[0].DestBlend = D3D11_BLEND_ONE;
	AddBlendingDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
	AddBlendingDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ZERO;
	AddBlendingDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ONE;
	AddBlendingDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
	AddBlendingDesc.RenderTarget[0].RenderTargetWriteMask = ((D3D11_COLOR_WRITE_ENABLE_RED | D3D11_COLOR_WRITE_ENABLE_GREEN) | D3D11_COLOR_WRITE_ENABLE_BLUE);
	AddBlendingDesc.AlphaToCoverageEnable = FALSE;
	V(device->CreateBlendState(&AddBlendingDesc, &AddBlending));

	D3D11_SAMPLER_DESC PointSamplerDesc;
	PointSamplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	PointSamplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
	PointSamplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
	PointSamplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
	PointSamplerDesc.MinLOD = -FLT_MAX;
	PointSamplerDesc.MaxLOD = FLT_MAX;
	PointSamplerDesc.MipLODBias = 0.0f;
	PointSamplerDesc.MaxAnisotropy = 1;
	PointSamplerDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
	PointSamplerDesc.BorderColor[0] = 1.0f;
	PointSamplerDesc.BorderColor[1] = 1.0f;
	PointSamplerDesc.BorderColor[2] = 1.0f;
	PointSamplerDesc.BorderColor[3] = 1.0f;
	PointSamplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
	V(device->CreateSamplerState(&PointSamplerDesc, &PointSampler));

	D3D11_SAMPLER_DESC LinearSamplerDesc;
	LinearSamplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	LinearSamplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
	LinearSamplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
	LinearSamplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
	LinearSamplerDesc.MinLOD = -FLT_MAX;
	LinearSamplerDesc.MaxLOD = FLT_MAX;
	LinearSamplerDesc.MipLODBias = 0.0f;
	LinearSamplerDesc.MaxAnisotropy = 1;
	LinearSamplerDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
	LinearSamplerDesc.BorderColor[0] = 1.0f;
	LinearSamplerDesc.BorderColor[1] = 1.0f;
	LinearSamplerDesc.BorderColor[2] = 1.0f;
	LinearSamplerDesc.BorderColor[3] = 1.0f;
	LinearSamplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	V(device->CreateSamplerState(&LinearSamplerDesc, &LinearSampler));

	quad = new Quad(device, SSS_Blur_VS_bytecode, sizeof(SSS_Blur_VS_bytecode));
}

SSSBlur::~SSSBlur()
{
	SAFE_DELETE(quad);
	SAFE_RELEASE(LinearSampler);
	SAFE_RELEASE(PointSampler);
	SAFE_RELEASE(AddBlending);
	SAFE_RELEASE(BlurStencil);
	SAFE_RELEASE(CbufUpdatedPerFrame);
	SAFE_RELEASE(SSS_Blur_PS);
	SAFE_RELEASE(SSS_VS);
}

void SSSBlur::go(ID3D11DeviceContext* context,
	ID3D11RenderTargetView* mainRTV,
	ID3D11ShaderResourceView* irradianceSRV,
	ID3D11ShaderResourceView* depthSRV,
	ID3D11DepthStencilView* depthDSV,
	ID3D11ShaderResourceView* albedoSRV)
{
	// Set variables:
	D3D11_MAPPED_SUBRESOURCE mappedResource;
	context->Map(CbufUpdatedPerFrame, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
	((struct UpdatedPerFrame*)mappedResource.pData)->currProj = camera.getProjectionMatrix();
	((struct UpdatedPerFrame*)mappedResource.pData)->scatteringDistance = m_scatteringDistance;
	((struct UpdatedPerFrame*)mappedResource.pData)->worldScale = m_worldScale;
	((struct UpdatedPerFrame*)mappedResource.pData)->postscatterEnabled = m_postscatterEnabled ? 1.0f : -1.0f;
	((struct UpdatedPerFrame*)mappedResource.pData)->sampleBudget = m_sampleBudget;
	((struct UpdatedPerFrame*)mappedResource.pData)->pixelsPerSample = m_pixelsPerSample;
	context->Unmap(CbufUpdatedPerFrame, 0);

	// Set input layout and viewport:
	quad->setInputLayout(context);

	UINT StencilRef = 1;

	context->PSSetShaderResources(TEX_IRRADIANCE, 1U, &irradianceSRV);
	context->PSSetShaderResources(TEX_ALBEDO, 1U, &albedoSRV);
	context->PSSetShaderResources(TEX_DEPTH, 1U, &depthSRV);
	context->VSSetConstantBuffers(CB_UPDATEDPERFRAME, 1U, &CbufUpdatedPerFrame);
	context->PSSetConstantBuffers(CB_UPDATEDPERFRAME, 1U, &CbufUpdatedPerFrame);
	context->PSSetSamplers(SAMP_POINT, 1, &PointSampler);
	context->PSSetSamplers(SAMP_LINEAR, 1, &LinearSampler);
	context->VSSetShader(SSS_VS, NULL, 0);
	context->GSSetShader(NULL, NULL, 0);
	context->PSSetShader(SSS_Blur_PS, NULL, 0);
	context->OMSetDepthStencilState(BlurStencil, StencilRef);
	FLOAT BlendFactor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
	context->OMSetBlendState(AddBlending, BlendFactor, 0xFFFFFFFF);

	context->OMSetRenderTargets(1, &mainRTV, depthDSV);
	quad->draw(context);
	ID3D11RenderTargetView* pRenderTargetViews[1] = { NULL };
	context->OMSetRenderTargets(1, pRenderTargetViews, NULL);

	ID3D11ShaderResourceView* pShaderResourceViews[4] = { NULL, NULL, NULL, NULL };
	context->PSSetShaderResources(0, 4, pShaderResourceViews);
}
