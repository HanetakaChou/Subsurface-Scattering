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

#include "FilmGrain.h"

#include "../../dxbc/FilmGrain_PassVS_bytecode.inl"
#include "../../dxbc/FilmGrain_FilmGrainPS_bytecode.inl"

#define TEX_SRC 0
#define SAMP_LINEAR 0

#include <sstream>
using namespace std;

FilmGrain::FilmGrain(ID3D11Device* device, int width, int height)
	: width(width),
	height(height),
	PassVS(NULL),
	FilmGrainPS(NULL),
	DisableDepthStencil(NULL),
	NoBlending(NULL),
	quad(NULL)
{

	HRESULT hr;

	V(device->CreateVertexShader(FilmGrain_PassVS_bytecode, sizeof(FilmGrain_PassVS_bytecode), NULL, &PassVS));
	V(device->CreatePixelShader(FilmGrain_FilmGrainPS_bytecode, sizeof(FilmGrain_FilmGrainPS_bytecode), NULL, &FilmGrainPS));

	D3D11_DEPTH_STENCIL_DESC DisableDepthStencilDesc = {};
	DisableDepthStencilDesc.DepthEnable = TRUE;
	DisableDepthStencilDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
	DisableDepthStencilDesc.DepthFunc = D3D11_COMPARISON_LESS;
	DisableDepthStencilDesc.StencilEnable = FALSE;
	DisableDepthStencilDesc.StencilReadMask = D3D11_DEFAULT_STENCIL_READ_MASK;
	DisableDepthStencilDesc.StencilWriteMask = D3D11_DEFAULT_STENCIL_WRITE_MASK;
	DisableDepthStencilDesc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
	DisableDepthStencilDesc.BackFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
	DisableDepthStencilDesc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
	DisableDepthStencilDesc.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
	DisableDepthStencilDesc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
	DisableDepthStencilDesc.BackFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
	DisableDepthStencilDesc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
	DisableDepthStencilDesc.BackFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
	DisableDepthStencilDesc.DepthEnable = FALSE;
	DisableDepthStencilDesc.StencilEnable = FALSE;
	V(device->CreateDepthStencilState(&DisableDepthStencilDesc, &DisableDepthStencil));

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

	quad = new Quad(device, FilmGrain_PassVS_bytecode, sizeof(FilmGrain_PassVS_bytecode));
}

FilmGrain::~FilmGrain()
{
	SAFE_DELETE(quad);
	SAFE_RELEASE(LinearSampler);
	SAFE_RELEASE(NoBlending);
	SAFE_RELEASE(DisableDepthStencil);
	SAFE_RELEASE(FilmGrainPS);
	SAFE_RELEASE(PassVS);
}

void FilmGrain::go(ID3D11DeviceContext* context, ID3D11ShaderResourceView* src, ID3D11RenderTargetView* dst)
{
	quad->setInputLayout(context);

	context->PSSetShaderResources(TEX_SRC, 1U, &src);

	D3D11_VIEWPORT viewport = Utils::viewportFromView(dst);
	context->RSSetViewports(1, &viewport);

	context->VSSetShader(PassVS, NULL, 0);
	context->GSSetShader(NULL, NULL, 0);
	context->PSSetShader(FilmGrainPS, NULL, 0);
	context->PSSetSamplers(SAMP_LINEAR, 1, &LinearSampler);
	context->OMSetDepthStencilState(DisableDepthStencil, 0);
	FLOAT BlendFactor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
	context->OMSetBlendState(NoBlending, BlendFactor, 0xFFFFFFFF);

	context->OMSetRenderTargets(1, &dst, NULL);
	quad->draw(context);
	ID3D11RenderTargetView* pRenderTargetViews[1] = { NULL };
	context->OMSetRenderTargets(1, pRenderTargetViews, NULL);

	ID3D11ShaderResourceView* pShaderResourceViews[1] = { NULL };
	context->PSSetShaderResources(TEX_SRC, 1U, pShaderResourceViews);
}
