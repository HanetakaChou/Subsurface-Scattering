#include "AddSpecular.h"

#include "../../dxbc/AddSpecular_PassVS_bytecode.inl"
#include "../../dxbc/AddSpecular_AddSpecularPS_bytecode.inl"

#define TEX_SPECULARS 0
#define SAMP_POINT 0

static ID3D11VertexShader *PassVS = NULL;
static ID3D11PixelShader *AddSpecularPS = NULL;
static ID3D11DepthStencilState *DisableDepthStencil = NULL;
static ID3D11BlendState *AddSpecularBlending = NULL;
static ID3D11SamplerState *PointSampler = NULL;
static Quad *quad = NULL;

void  initAddSpecular(ID3D11Device *device)
{
    HRESULT hr;

    V(device->CreateVertexShader(AddSpecular_PassVS_bytecode, sizeof(AddSpecular_PassVS_bytecode), NULL, &PassVS));
    V(device->CreatePixelShader(AddSpecular_AddSpecularPS_bytecode, sizeof(AddSpecular_AddSpecularPS_bytecode), NULL, &AddSpecularPS));

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

    D3D11_BLEND_DESC AddSpecularBlendingDesc = {};
    AddSpecularBlendingDesc.AlphaToCoverageEnable = FALSE;
    AddSpecularBlendingDesc.IndependentBlendEnable = FALSE;
    AddSpecularBlendingDesc.RenderTarget[0].BlendEnable = FALSE;
    AddSpecularBlendingDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_ONE;
    AddSpecularBlendingDesc.RenderTarget[0].DestBlend = D3D11_BLEND_ZERO;
    AddSpecularBlendingDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
    AddSpecularBlendingDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
    AddSpecularBlendingDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
    AddSpecularBlendingDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
    AddSpecularBlendingDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
    AddSpecularBlendingDesc.RenderTarget[0].BlendEnable = TRUE;
    AddSpecularBlendingDesc.RenderTarget[1].BlendEnable = FALSE;
    AddSpecularBlendingDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_ONE;
    AddSpecularBlendingDesc.RenderTarget[0].DestBlend = D3D11_BLEND_ONE;
    AddSpecularBlendingDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
    AddSpecularBlendingDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
    AddSpecularBlendingDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
    AddSpecularBlendingDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
    AddSpecularBlendingDesc.RenderTarget[0].RenderTargetWriteMask = ((D3D11_COLOR_WRITE_ENABLE_RED | D3D11_COLOR_WRITE_ENABLE_GREEN) | D3D11_COLOR_WRITE_ENABLE_BLUE);
    AddSpecularBlendingDesc.AlphaToCoverageEnable = FALSE;
    V(device->CreateBlendState(&AddSpecularBlendingDesc, &AddSpecularBlending));

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

    quad = new Quad(device, AddSpecular_PassVS_bytecode, sizeof(AddSpecular_PassVS_bytecode));
}

void releaseAddSpecular()
{
    SAFE_DELETE(quad);
    SAFE_RELEASE(PointSampler);
    SAFE_RELEASE(AddSpecularBlending);
    SAFE_RELEASE(DisableDepthStencil);
    SAFE_RELEASE(AddSpecularPS);
    SAFE_RELEASE(PassVS);
}

void addSpecular(ID3D11DeviceContext *context, ID3D11RenderTargetView *dst, ID3D11ShaderResourceView *specularsRT)
{
    quad->setInputLayout(context);

    context->PSSetShaderResources(TEX_SPECULARS, 1U, &specularsRT);

    context->VSSetShader(PassVS, NULL, 0);
    context->GSSetShader(NULL, NULL, 0);
    context->PSSetShader(AddSpecularPS, NULL, 0);
    context->PSSetSamplers(SAMP_POINT, 1, &PointSampler);
    context->OMSetDepthStencilState(DisableDepthStencil, 0);
    FLOAT BlendFactor[4] = {0.0f, 0.0f, 0.0f, 0.0f};
    context->OMSetBlendState(AddSpecularBlending, BlendFactor, 0xFFFFFFFF);

    context->OMSetRenderTargets(1, &dst, NULL);
    quad->draw(context);
    ID3D11RenderTargetView *pRenderTargetViews[1] = {NULL};
    context->OMSetRenderTargets(1, pRenderTargetViews, NULL);

    ID3D11ShaderResourceView *pShaderResourceViews[1] = {NULL};
    context->PSSetShaderResources(TEX_SPECULARS, 1U, pShaderResourceViews);
}
