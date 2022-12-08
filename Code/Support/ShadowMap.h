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


#ifndef _SHADOWMAP_H_
#define _SHADOWMAP_H_ 1

#include <sdkddkver.h>
#define NOMINMAX 1
#define WIN32_LEAN_AND_MEAN 1
#include <DXUT.h>
#include "RenderTarget.h"
#include <DirectXMath.h>

class ShadowMap {
public:
	static void init(ID3D11Device* device);
	static void release();

	ShadowMap(ID3D11Device* device, int width, int height);
	~ShadowMap();

	void begin(ID3D11DeviceContext* context, const DirectX::XMFLOAT4X4& view, const DirectX::XMFLOAT4X4& projection);
	void setWorldMatrix(ID3D11DeviceContext* context, const DirectX::XMFLOAT4X4& world);
	void end(ID3D11DeviceContext* context);

	operator ID3D11ShaderResourceView* const () { return *depthStencil; }

	static DirectX::XMFLOAT4X4 getViewProjectionTextureMatrix(const DirectX::XMFLOAT4X4& view, const DirectX::XMFLOAT4X4& projection);

private:
	DepthStencil* depthStencil;
	D3D11_VIEWPORT viewport;

	static ID3D11VertexShader* ShadowMapVS;
	static ID3D11Buffer* CbufUpdatedPerFrame;
	static ID3D11Buffer* CbufUpdatedPerObject;
	static ID3D11DepthStencilState* EnableDepthDisableStencil;
	static ID3D11BlendState* NoBlending;
	static ID3D11InputLayout* vertexLayout;
};

void shadowPass(ID3D11DeviceContext* context);

#endif
