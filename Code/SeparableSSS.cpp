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

#include <sstream>
#include "SeparableSSS.h"
#include "Demo.h"

#include "../../dxbc/SeparableSSS_SSSSBlurVS_bytecode.inl"
#include "../../dxbc/SeparableSSS_SSSSBlurXPS_bytecode.inl"
#include "../../dxbc/SeparableSSS_SSSSBlurYPS_bytecode.inl"

struct UpdatedPerFrame
{
    __declspec(align(16)) DirectX::XMFLOAT4X4 currProj;
    __declspec(align(16)) DirectX::XMFLOAT4 kernel[33];
    float sssWidth;
    int nSamples;
};

#define CB_UPDATEDPERFRAME 0
#define TEX_STRENGTH 0
#define TEX_COLOR 1
#define TEX_DEPTH 2
#define SAMP_POINT 0
#define SAMP_LINEAR 1

using namespace std;

SeparableSSS::SeparableSSS(ID3D11Device *device,
                           int width,
                           int height,
                           float sssWidth,
                           int nSamples,
                           DXGI_FORMAT format) : sssWidth(sssWidth),
                                                 nSamples(nSamples),
                                                 stencilInitialized(stencilInitialized),
                                                 strength(DirectX::XMFLOAT3(0.48f, 0.41f, 0.28f)),
                                                 falloff(DirectX::XMFLOAT3(1.0f, 0.37f, 0.3f))
{
    HRESULT hr;

    assert(nSamples <= 33);

    D3D11_BUFFER_DESC UpdatedPerFrameDesc =
        {
            sizeof(struct UpdatedPerFrame),
            D3D11_USAGE_DYNAMIC,
            D3D11_BIND_CONSTANT_BUFFER,
            D3D11_CPU_ACCESS_WRITE,
        };
    V(device->CreateBuffer(&UpdatedPerFrameDesc, NULL, &CbufUpdatedPerFrame));

    V(device->CreateVertexShader(SeparableSSS_SSSSBlurVS_bytecode, sizeof(SeparableSSS_SSSSBlurVS_bytecode), NULL, &SSSSBlurVS));
    V(device->CreatePixelShader(SeparableSSS_SSSSBlurXPS_bytecode, sizeof(SeparableSSS_SSSSBlurXPS_bytecode), NULL, &SSSSBlurXPS));
    V(device->CreatePixelShader(SeparableSSS_SSSSBlurYPS_bytecode, sizeof(SeparableSSS_SSSSBlurYPS_bytecode), NULL, &SSSSBlurYPS));

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
    NoBlendingDesc.RenderTarget[1].BlendEnable = FALSE;
    V(device->CreateBlendState(&NoBlendingDesc, &NoBlending));

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

    quad = new Quad(device, SeparableSSS_SSSSBlurVS_bytecode, sizeof(SeparableSSS_SSSSBlurVS_bytecode));

    // Create the temporal render target:
    tmpRT = new RenderTarget(device, width, height, format);

    // And finally, calculate the sample positions and weights:
    calculateKernel();
}

SeparableSSS::~SeparableSSS()
{
    SAFE_DELETE(tmpRT);
    SAFE_DELETE(quad);
    SAFE_RELEASE(LinearSampler);
    SAFE_RELEASE(PointSampler);
    SAFE_RELEASE(NoBlending);
    SAFE_RELEASE(BlurStencil);
    SAFE_RELEASE(CbufUpdatedPerFrame);
    SAFE_RELEASE(SSSSBlurYPS);
    SAFE_RELEASE(SSSSBlurXPS);
    SAFE_RELEASE(SSSSBlurVS);
}

DirectX::XMFLOAT3 SeparableSSS::gaussian(float variance, float r)
{
    /**
     * We use a falloff to modulate the shape of the profile. Big falloffs
     * spreads the shape making it wider, while small falloffs make it
     * narrower.
     */
    DirectX::XMFLOAT3 g;
    float rr_x = r / (0.001f + falloff.x);
    float rr_y = r / (0.001f + falloff.y);
    float rr_z = r / (0.001f + falloff.z);
    g.x = exp((-(rr_x * rr_x)) / (2.0f * variance)) / (2.0f * 3.14f * variance);
    g.y = exp((-(rr_y * rr_y)) / (2.0f * variance)) / (2.0f * 3.14f * variance);
    g.z = exp((-(rr_z * rr_z)) / (2.0f * variance)) / (2.0f * 3.14f * variance);
    return g;
}

DirectX::XMFLOAT3 SeparableSSS::profile(float r)
{
    /**
     * We used the red channel of the original skin profile defined in
     * [d'Eon07] for all three channels. We noticed it can be used for green
     * and blue channels (scaled using the falloff parameter) without
     * introducing noticeable differences and allowing for total control over
     * the profile. For example, it allows to create blue SSS gradients, which
     * could be useful in case of rendering blue creatures.
     */
    
    // 0.233f * gaussian(0.0064f, r) + /* We consider this one to be directly bounced light, accounted by the strength parameter (see @STRENGTH) */
    DirectX::XMFLOAT3 g_1 = gaussian(0.0484f, r);
    DirectX::XMFLOAT3 g_2 = gaussian(0.187f, r);
    DirectX::XMFLOAT3 g_3 = gaussian(0.567f, r);
    DirectX::XMFLOAT3 g_4 = gaussian(1.99f, r);
    DirectX::XMFLOAT3 g_5 = gaussian(7.41f, r);

    DirectX::XMFLOAT3 g;
    g.x = 0.100f * g_1.x + 0.118f * g_2.x + 0.113f * g_3.x + 0.358f * g_4.x + 0.078f * g_5.x;
    g.y = 0.100f * g_1.y + 0.118f * g_2.y + 0.113f * g_3.y + 0.358f * g_4.y + 0.078f * g_5.y;
    g.z = 0.100f * g_1.z + 0.118f * g_2.z + 0.113f * g_3.z + 0.358f * g_4.z + 0.078f * g_5.z;
    return g;
}

void SeparableSSS::calculateKernel()
{
    const float RANGE = nSamples > 20 ? 3.0f : 2.0f;
    const float EXPONENT = 2.0f;

    kernel.resize(nSamples);

    // Calculate the offsets:
    float step = 2.0f * RANGE / (nSamples - 1);
    for (int i = 0; i < nSamples; i++)
    {
        float o = -RANGE + float(i) * step;
        float sign = o < 0.0f ? -1.0f : 1.0f;
        kernel[i].w = RANGE * sign * abs(pow(o, EXPONENT)) / pow(RANGE, EXPONENT);
    }

    // Calculate the weights:
    for (int i = 0; i < nSamples; i++)
    {
        float w0 = i > 0 ? abs(kernel[i].w - kernel[i - 1].w) : 0.0f;
        float w1 = i < nSamples - 1 ? abs(kernel[i].w - kernel[i + 1].w) : 0.0f;
        float area = (w0 + w1) / 2.0f;
        DirectX::XMFLOAT3 r = profile(kernel[i].w);
        kernel[i].x = area * r.x;
        kernel[i].y = area * r.y;
        kernel[i].z = area * r.z;
    }

    // We want the offset 0.0 to come first:
    DirectX::XMFLOAT4 t = kernel[nSamples / 2];
    for (int i = nSamples / 2; i > 0; i--)
        kernel[i] = kernel[i - 1];
    kernel[0] = t;

    // Calculate the sum of the weights, we will need to normalize them below:
    DirectX::XMFLOAT3 sum = DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f);
    for (int i = 0; i < nSamples; i++)
    {
        sum.x += kernel[i].x;
        sum.y += kernel[i].y;
        sum.z += kernel[i].z;
    }

    // Normalize the weights:
    for (int i = 0; i < nSamples; i++)
    {
        kernel[i].x /= sum.x;
        kernel[i].y /= sum.y;
        kernel[i].z /= sum.z;
    }

    // Tweak them using the desired strength. The first one is:
    //     lerp(1.0, kernel[0].rgb, strength)
    kernel[0].x = (1.0f - strength.x) * 1.0f + strength.x * kernel[0].x;
    kernel[0].y = (1.0f - strength.y) * 1.0f + strength.y * kernel[0].y;
    kernel[0].z = (1.0f - strength.z) * 1.0f + strength.z * kernel[0].z;

    // The others:
    //     lerp(0.0, kernel[0].rgb, strength)
    for (int i = 1; i < nSamples; i++)
    {
        kernel[i].x *= strength.x;
        kernel[i].y *= strength.y;
        kernel[i].z *= strength.z;
    }
}

void SeparableSSS::go(ID3D11DeviceContext *context,
                      ID3D11RenderTargetView *mainRTV,
                      ID3D11ShaderResourceView *mainSRV,
                      ID3D11ShaderResourceView *depthSRV,
                      ID3D11DepthStencilView *depthDSV,
                      ID3D11ShaderResourceView *stregthSRV)
{
    // Clear the temporal render target:
    float clearColor[4] = {0.0f, 0.0f, 0.0f, 0.0f};
    context->ClearRenderTargetView(*tmpRT, clearColor);

    // Clear the stencil buffer if it was not available, and thus one must be
    // initialized on the fly:
    if (!stencilInitialized)
        context->ClearDepthStencilView(depthDSV, D3D11_CLEAR_STENCIL, 1.0, 0);

    // Set variables:
    D3D11_MAPPED_SUBRESOURCE mappedResource;
    context->Map(CbufUpdatedPerFrame, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
    ((struct UpdatedPerFrame *)mappedResource.pData)->currProj = camera.getProjectionMatrix();
    memcpy(((struct UpdatedPerFrame *)mappedResource.pData)->kernel, &kernel[0], sizeof(DirectX::XMFLOAT4) * kernel.size());
    ((struct UpdatedPerFrame *)mappedResource.pData)->sssWidth = sssWidth;
    ((struct UpdatedPerFrame *)mappedResource.pData)->nSamples = nSamples;
    context->Unmap(CbufUpdatedPerFrame, 0);

    context->PSSetShaderResources(TEX_STRENGTH, 1U, &stregthSRV);
    context->PSSetShaderResources(TEX_DEPTH, 1U, &depthSRV);

    // Set input layout and viewport:
    quad->setInputLayout(context);
    tmpRT->setViewport(context);

    UINT StencilRef = 1;

    // Run the horizontal pass:
    {
        context->PSSetShaderResources(TEX_COLOR, 1U, &mainSRV);
        context->VSSetConstantBuffers(CB_UPDATEDPERFRAME, 1U, &CbufUpdatedPerFrame);
        context->PSSetConstantBuffers(CB_UPDATEDPERFRAME, 1U, &CbufUpdatedPerFrame);
        context->PSSetSamplers(SAMP_POINT, 1, &PointSampler);
        context->PSSetSamplers(SAMP_LINEAR, 1, &LinearSampler);
        context->VSSetShader(SSSSBlurVS, NULL, 0);
        context->GSSetShader(NULL, NULL, 0);
        context->PSSetShader(SSSSBlurXPS, NULL, 0);
        context->OMSetDepthStencilState(BlurStencil, StencilRef);
        FLOAT BlendFactor[4] = {0.0f, 0.0f, 0.0f, 0.0f};
        context->OMSetBlendState(NoBlending, BlendFactor, 0xFFFFFFFF);

        context->OMSetRenderTargets(1, *tmpRT, depthDSV);
        quad->draw(context);
        ID3D11RenderTargetView *pRenderTargetViews[1] = {NULL};
        context->OMSetRenderTargets(1, pRenderTargetViews, NULL);
    }

    // And finish with the vertical one:
    {
        ID3D11ShaderResourceView *tmpRTSRV = *tmpRT;
        context->PSSetShaderResources(TEX_COLOR, 1U, &tmpRTSRV);
        context->VSSetConstantBuffers(CB_UPDATEDPERFRAME, 1U, &CbufUpdatedPerFrame);
        context->PSSetConstantBuffers(CB_UPDATEDPERFRAME, 1U, &CbufUpdatedPerFrame);
        context->PSSetSamplers(SAMP_POINT, 1, &PointSampler);
        context->PSSetSamplers(SAMP_LINEAR, 1, &LinearSampler);
        context->VSSetShader(SSSSBlurVS, NULL, 0);
        context->GSSetShader(NULL, NULL, 0);
        context->PSSetShader(SSSSBlurYPS, NULL, 0);
        context->OMSetDepthStencilState(BlurStencil, StencilRef);
        FLOAT BlendFactor[4] = {0.0f, 0.0f, 0.0f, 0.0f};
        context->OMSetBlendState(NoBlending, BlendFactor, 0xFFFFFFFF);

        context->OMSetRenderTargets(1, &mainRTV, depthDSV);
        quad->draw(context);
        ID3D11RenderTargetView *pRenderTargetViews[1] = {NULL};
        context->OMSetRenderTargets(1, pRenderTargetViews, NULL);
    }

    ID3D11ShaderResourceView *pShaderResourceViews[4] = {NULL, NULL, NULL, NULL};
    context->PSSetShaderResources(0, 4, pShaderResourceViews);
}

string SeparableSSS::getKernelCode() const
{
    stringstream s;
    s << "float4 kernel[] = {"
      << "\r\n";
    for (int i = 0; i < nSamples; i++)
        s << "    float4(" << kernel[i].x << ", "
          << kernel[i].y << ", "
          << kernel[i].z << ", "
          << kernel[i].w << "),"
          << "\r\n";
    s << "};"
      << "\r\n";
    return s.str();
}
