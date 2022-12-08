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


#include "ShadowMap.h"
#include "../Demo.h"

#include "../../dxbc/ShadowMap_ShadowMapVS_bytecode.inl"

ID3D11VertexShader* ShadowMap::ShadowMapVS = NULL;
ID3D11Buffer* ShadowMap::CbufUpdatedPerFrame = NULL;
ID3D11Buffer* ShadowMap::CbufUpdatedPerObject = NULL;
ID3D11DepthStencilState* ShadowMap::EnableDepthDisableStencil = NULL;
ID3D11BlendState* ShadowMap::NoBlending = NULL;
ID3D11InputLayout* ShadowMap::vertexLayout = NULL;

#define CB_UPDATEDPERFRAME		0
#define CB_UPDATEDPEROBJECT		1

struct UpdatedPerFrame
{
	__declspec(align(16)) DirectX::XMFLOAT4X4 view;
	__declspec(align(16)) DirectX::XMFLOAT4X4 projection;
};

struct UpdatedPerObject
{
	__declspec(align(16)) DirectX::XMFLOAT4X4 world;
};


void ShadowMap::init(ID3D11Device* device) {
	HRESULT hr;

	D3D11_BUFFER_DESC UpdatedPerFrameDesc =
	{
	   sizeof(struct UpdatedPerFrame),
		D3D11_USAGE_DYNAMIC,
		D3D11_BIND_CONSTANT_BUFFER,
		D3D11_CPU_ACCESS_WRITE,
	};
	V(device->CreateBuffer(&UpdatedPerFrameDesc, NULL, &CbufUpdatedPerFrame));

	D3D11_BUFFER_DESC UpdatedPerObjectDesc =
	{
	   sizeof(struct UpdatedPerObject),
		D3D11_USAGE_DYNAMIC,
		D3D11_BIND_CONSTANT_BUFFER,
		D3D11_CPU_ACCESS_WRITE,
	};
	V(device->CreateBuffer(&UpdatedPerObjectDesc, NULL, &CbufUpdatedPerObject));

	V(device->CreateVertexShader(ShadowMap_ShadowMapVS_bytecode, sizeof(ShadowMap_ShadowMapVS_bytecode), NULL, &ShadowMapVS));

	D3D11_DEPTH_STENCIL_DESC EnableDepthDisableStencilDesc = { };
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
	EnableDepthDisableStencilDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
	EnableDepthDisableStencilDesc.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;
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

	const D3D11_INPUT_ELEMENT_DESC layout[] = {
		{ "POSITION",  0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 }
	};
	UINT numElements = sizeof(layout) / sizeof(D3D11_INPUT_ELEMENT_DESC);

	V(device->CreateInputLayout(layout, numElements, ShadowMap_ShadowMapVS_bytecode, sizeof(ShadowMap_ShadowMapVS_bytecode), &vertexLayout));
}


void ShadowMap::release() {
	SAFE_RELEASE(vertexLayout);
	SAFE_RELEASE(NoBlending);
	SAFE_RELEASE(EnableDepthDisableStencil);
	SAFE_RELEASE(CbufUpdatedPerObject);
	SAFE_RELEASE(CbufUpdatedPerFrame);
	SAFE_RELEASE(ShadowMapVS);
}


ShadowMap::ShadowMap(ID3D11Device* device, int width, int height)
{
	depthStencil = new DepthStencil(device, width, height);
}


ShadowMap::~ShadowMap() {
	SAFE_DELETE(depthStencil);
}


void ShadowMap::begin(ID3D11DeviceContext* context, const DirectX::XMFLOAT4X4& view, const DirectX::XMFLOAT4X4& projection)
{
	context->IASetInputLayout(vertexLayout);

	context->ClearDepthStencilView(*depthStencil, D3D11_CLEAR_DEPTH, 1.0, 0);

	D3D11_MAPPED_SUBRESOURCE mappedResource;
	context->Map(CbufUpdatedPerFrame, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
	((struct UpdatedPerFrame*)mappedResource.pData)->view = view;
	((struct UpdatedPerFrame*)mappedResource.pData)->projection = projection;
	context->Unmap(CbufUpdatedPerFrame, 0);

	ID3D11RenderTargetView* pRenderTargetViews[1] = { NULL };
	context->OMSetRenderTargets(1, pRenderTargetViews, *depthStencil);

	UINT numViewports = 1;
	context->RSGetViewports(&numViewports, &viewport);
	depthStencil->setViewport(context);

	context->VSSetConstantBuffers(CB_UPDATEDPERFRAME, 1U, &CbufUpdatedPerFrame);
	context->VSSetConstantBuffers(CB_UPDATEDPEROBJECT, 1U, &CbufUpdatedPerObject);
	context->VSSetShader(ShadowMapVS, NULL, 0);
	context->GSSetShader(NULL, NULL, 0);
	context->PSSetShader(NULL, NULL, 0);
	context->OMSetDepthStencilState(EnableDepthDisableStencil, 0);
	FLOAT BlendFactor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
	context->OMSetBlendState(NoBlending, BlendFactor, 0xFFFFFFFF);
}


void ShadowMap::setWorldMatrix(ID3D11DeviceContext* context, const DirectX::XMFLOAT4X4& world) {
	D3D11_MAPPED_SUBRESOURCE mappedResource;
	context->Map(CbufUpdatedPerObject, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
	((struct UpdatedPerObject*)mappedResource.pData)->world = world;
	context->Unmap(CbufUpdatedPerObject, 0);
}

void ShadowMap::end(ID3D11DeviceContext* context) {
	context->RSSetViewports(1, &viewport);

	ID3D11RenderTargetView* pRenderTargetViews[1] = { NULL };
	context->OMSetRenderTargets(1, pRenderTargetViews, NULL);
}


DirectX::XMFLOAT4X4 ShadowMap::getViewProjectionTextureMatrix(const DirectX::XMFLOAT4X4& view, const DirectX::XMFLOAT4X4& projection) {
	
	DirectX::XMMATRIX scale = DirectX::XMMatrixScaling(0.5f, -0.5f, 1.0f);

	DirectX::XMMATRIX translation = DirectX::XMMatrixTranslation(0.5f, 0.5f, 0.0f);

	DirectX::XMFLOAT4X4 viewProjectionTexture;
	DirectX::XMStoreFloat4x4(&viewProjectionTexture, DirectX::XMMatrixMultiply(DirectX::XMMatrixMultiply(DirectX::XMMatrixMultiply(DirectX::XMLoadFloat4x4(&view), DirectX::XMLoadFloat4x4(&projection)), scale), translation));
	return viewProjectionTexture;
}


void shadowPass(ID3D11DeviceContext* context)
{
	for (int i = 0; i < N_LIGHTS; i++)
	{
		if (DirectX::XMVectorGetX(DirectX::XMVector3Length(DirectX::XMLoadFloat3(&lights[i].color))) > 0.0f)
		{
			lights[i].shadowMap->begin(context, lights[i].camera.getViewMatrix(), lights[i].camera.getProjectionMatrix());
			for (int j = 0; j < N_HEADS; j++)
			{
				DirectX::XMFLOAT4X4 world;
				DirectX::XMStoreFloat4x4(&world, DirectX::XMMatrixTranslation(j - (N_HEADS - 1) / 2.0f, 0.0f, 0.0f));

				lights[i].shadowMap->setWorldMatrix(context, world);

				mesh.Render(context, 0U, 0U, 0U);
			}
			lights[i].shadowMap->end(context);
		}
	}
}