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

#include "SkyDome.h"

#include "../../dxbc/SkyDome_SkyDomeVS_bytecode.inl"
#include "../../dxbc/SkyDome_SkyDomePS_bytecode.inl"

#include <sstream>
using namespace std;

struct UpdatedPerFrame
{
	__declspec(align(16)) DirectX::XMFLOAT4X4 view;
	__declspec(align(16)) DirectX::XMFLOAT4X4 projection;
	__declspec(align(16)) DirectX::XMFLOAT4X4 world;
	float intensity;
};

#define CB_UPDATEDPERFRAME 0
#define TEX_SKY 0
#define TEX_NORMAL 1
#define TEX_SPECULAR 2
#define SAMP_LINEAR 0

SkyDome::SkyDome(ID3D11Device* device, const std::wstring& dir, float intensity)
	: intensity(intensity),
	angle(0.0f, 0.0f)
{
	HRESULT hr;

	V(device->CreateVertexShader(SkyDome_SkyDomeVS_bytecode, sizeof(SkyDome_SkyDomeVS_bytecode), NULL, &SkyDomeVS));
	V(device->CreatePixelShader(SkyDome_SkyDomePS_bytecode, sizeof(SkyDome_SkyDomePS_bytecode), NULL, &SkyDomePS));

	D3D11_BUFFER_DESC UpdatedPerFrameDesc =
	{
		sizeof(struct UpdatedPerFrame),
		D3D11_USAGE_DYNAMIC,
		D3D11_BIND_CONSTANT_BUFFER,
		D3D11_CPU_ACCESS_WRITE,
	};
	V(device->CreateBuffer(&UpdatedPerFrameDesc, NULL, &CbufUpdatedPerFrame));

	D3D11_DEPTH_STENCIL_DESC EnableDepthDisableStencilDesc = {};
	EnableDepthDisableStencilDesc.DepthEnable = TRUE;
	EnableDepthDisableStencilDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
	EnableDepthDisableStencilDesc.DepthFunc = D3D11_COMPARISON_LESS;
	EnableDepthDisableStencilDesc.StencilEnable = FALSE;
	EnableDepthDisableStencilDesc.StencilReadMask = D3D11_DEFAULT_STENCIL_READ_MASK;
	EnableDepthDisableStencilDesc.StencilWriteMask = D3D11_DEFAULT_STENCIL_WRITE_MASK;
	EnableDepthDisableStencilDesc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
	EnableDepthDisableStencilDesc.BackFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
	EnableDepthDisableStencilDesc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
	EnableDepthDisableStencilDesc.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
	EnableDepthDisableStencilDesc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
	EnableDepthDisableStencilDesc.BackFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
	EnableDepthDisableStencilDesc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
	EnableDepthDisableStencilDesc.BackFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
	EnableDepthDisableStencilDesc.DepthEnable = TRUE;
	EnableDepthDisableStencilDesc.StencilEnable = FALSE;
	V(device->CreateDepthStencilState(&EnableDepthDisableStencilDesc, &EnableDepthDisableStencil));

	D3D11_BLEND_DESC NoBlendingDesc = {};
	NoBlendingDesc.AlphaToCoverageEnable = FALSE;
	NoBlendingDesc.IndependentBlendEnable = FALSE;
	NoBlendingDesc.RenderTarget[0].BlendEnable = FALSE;
	NoBlendingDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_ONE;
	NoBlendingDesc.RenderTarget[0].DestBlend = D3D11_BLEND_ZERO;
	NoBlendingDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
	NoBlendingDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
	NoBlendingDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
	NoBlendingDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
	NoBlendingDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
	NoBlendingDesc.AlphaToCoverageEnable = FALSE;
	NoBlendingDesc.RenderTarget[0].BlendEnable = FALSE;
	V(device->CreateBlendState(&NoBlendingDesc, &NoBlending));

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

	createMesh(device, dir);
}

SkyDome::~SkyDome()
{
	SAFE_RELEASE(LinearSampler);
	SAFE_RELEASE(NoBlending);
	SAFE_RELEASE(EnableDepthDisableStencil);
	SAFE_RELEASE(CbufUpdatedPerFrame);
	SAFE_RELEASE(SkyDomePS);
	SAFE_RELEASE(SkyDomeVS);
	mesh.Destroy();
}

void SkyDome::render(ID3D11DeviceContext* context, const DirectX::XMFLOAT4X4& view, const DirectX::XMFLOAT4X4& projection, float scale)
{
	DirectX::XMFLOAT4X4 world;
	DirectX::XMStoreFloat4x4(&world, DirectX::XMMatrixScaling(scale, scale, scale));

	DirectX::XMMATRIX t = DirectX::XMMatrixRotationX(angle.y);
	DirectX::XMStoreFloat4x4(&world, DirectX::XMMatrixMultiply(t, DirectX::XMLoadFloat4x4(&world)));

	t = DirectX::XMMatrixRotationY(angle.x);
	DirectX::XMStoreFloat4x4(&world, DirectX::XMMatrixMultiply(t, DirectX::XMLoadFloat4x4(&world)));

	D3D11_MAPPED_SUBRESOURCE mappedResource;
	context->Map(CbufUpdatedPerFrame, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
	((struct UpdatedPerFrame*)mappedResource.pData)->view = view;
	((struct UpdatedPerFrame*)mappedResource.pData)->projection = projection;
	((struct UpdatedPerFrame*)mappedResource.pData)->world = world;
	((struct UpdatedPerFrame*)mappedResource.pData)->intensity = intensity;
	context->Unmap(CbufUpdatedPerFrame, 0);

	context->VSSetConstantBuffers(CB_UPDATEDPERFRAME, 1U, &CbufUpdatedPerFrame);
	context->PSSetConstantBuffers(CB_UPDATEDPERFRAME, 1U, &CbufUpdatedPerFrame);
	context->VSSetShader(SkyDomeVS, NULL, 0);
	context->GSSetShader(NULL, NULL, 0);
	context->PSSetShader(SkyDomePS, NULL, 0);
	context->PSSetSamplers(SAMP_LINEAR, 1, &LinearSampler);
	context->OMSetDepthStencilState(EnableDepthDisableStencil, 0);
	FLOAT BlendFactor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
	context->OMSetBlendState(NoBlending, BlendFactor, 0xFFFFFFFF);

	mesh.Render(context, TEX_SKY, TEX_NORMAL, TEX_SPECULAR);
}

void SkyDome::createMesh(ID3D11Device* device, const std::wstring& dir)
{
	HRESULT hr;

	WCHAR strPath[512];
	V(DXUTFindDXSDKMediaFileCch(strPath, _countof(strPath), (dir + L"\\SkyDome.sdkmesh").c_str()));
	V(mesh.Create(device, strPath, NULL));
}
