#include "Main.h"
#include "../Demo.h"
#include <vector>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <limits>

#include "../../dxbc/Main_RenderVS_bytecode.inl"
#include "../../dxbc/Main_RenderPS_bytecode.inl"

static ID3D11Buffer *CbufUpdatedPerFrame = NULL;
static ID3D11Buffer *CbufUpdatedPerObject = NULL;
static ID3D11DepthStencilState *EnableDepthDisableStencil = NULL;
static ID3D11BlendState *NoBlending = NULL;
static ID3D11RasterizerState *EnableMultisampling = NULL;
static ID3D11VertexShader *RenderVS = NULL;
static ID3D11PixelShader *RenderPS = NULL;
static ID3D11InputLayout *vertexLayout = NULL;
static ID3D11SamplerState *PointSampler = NULL;
static ID3D11SamplerState *LinearSampler = NULL;
static ID3D11SamplerState *AnisotropicSampler = NULL;
static ID3D11SamplerState *ShadowSampler = NULL;

static ID3D11ShaderResourceView *specularAOSRV = NULL;
static ID3D11ShaderResourceView *beckmannSRV = NULL;
static ID3D11ShaderResourceView *irradianceSRV = NULL;

#define CB_UPDATEDPERFRAME 0
#define CB_UPDATEDPEROBJECT 1

#define TEX_DIFFUSE 0
#define TEX_NORMAL 1
#define TEX_SPECULAR 2
#define TEX_SPECULARAO 3
#define TEX_BECKMANN 4
#define TEX_IRRADIANCE 5
#define TEX_SHADOW_MAPS 6

#define SAMP_POINT 0
#define SAMP_LINEAR 1
#define SAMP_ANISOTROPIS 2
#define SAMP_SHADOW 3

struct UpdatedPerFrame
{
    __declspec(align(16)) DirectX::XMFLOAT3 cameraPosition;
    __declspec(align(16)) DirectX::XMFLOAT4X4 currProj;
    struct
    {
        __declspec(align(16)) DirectX::XMFLOAT4X4 viewProjection;
        __declspec(align(16)) DirectX::XMFLOAT4X4 projection;
        __declspec(align(16)) DirectX::XMFLOAT3 position;
        __declspec(align(16)) DirectX::XMFLOAT3 direction;
        __declspec(align(16)) DirectX::XMFLOAT4 color_attenuation;
        float falloffStart;
        float falloffWidth;
        float farPlane;
        float bias;
    } lights[N_LIGHTS];
};

static struct UpdatedPerFrame mainEffect_UpdatedPerFrame;

struct UpdatedPerObject
{
    __declspec(align(16)) DirectX::XMFLOAT4X4 currWorldViewProj;
    __declspec(align(16)) DirectX::XMFLOAT4X4 world;
    __declspec(align(16)) DirectX::XMFLOAT4X4 worldInverseTranspose;
    float bumpiness;
    float specularIntensity;
    float specularRoughness;
    float specularFresnel;
    float translucency;
    float sssEnabled;
    float sssWidth;
    float ambient;
};

static struct UpdatedPerObject mainEffect_UpdatedPerObject;

void initMainEffect(ID3D11Device *device, ID3D11ShaderResourceView *l_specularAOSRV, ID3D11ShaderResourceView *l_beckmannSRV, ID3D11ShaderResourceView *l_irradianceSRV)
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

    D3D11_BUFFER_DESC UpdatedPerObjectDesc =
        {
            sizeof(struct UpdatedPerObject),
            D3D11_USAGE_DYNAMIC,
            D3D11_BIND_CONSTANT_BUFFER,
            D3D11_CPU_ACCESS_WRITE,
        };
    V(device->CreateBuffer(&UpdatedPerObjectDesc, NULL, &CbufUpdatedPerObject));

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
    EnableDepthDisableStencilDesc.StencilEnable = TRUE;
    EnableDepthDisableStencilDesc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_REPLACE;
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
    NoBlendingDesc.RenderTarget[1].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
    NoBlendingDesc.RenderTarget[2].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
    NoBlendingDesc.RenderTarget[3].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
    NoBlendingDesc.AlphaToCoverageEnable = FALSE;
    NoBlendingDesc.RenderTarget[0].BlendEnable = FALSE;
    NoBlendingDesc.RenderTarget[1].BlendEnable = FALSE;
    NoBlendingDesc.RenderTarget[2].BlendEnable = FALSE;
    NoBlendingDesc.RenderTarget[3].BlendEnable = FALSE;
    V(device->CreateBlendState(&NoBlendingDesc, &NoBlending));

    D3D11_RASTERIZER_DESC EnableMultisamplingDesc;
    EnableMultisamplingDesc.FillMode = D3D11_FILL_SOLID;
    EnableMultisamplingDesc.CullMode = D3D11_CULL_BACK;
    EnableMultisamplingDesc.FrontCounterClockwise = FALSE;
    EnableMultisamplingDesc.DepthBias = 0;
    EnableMultisamplingDesc.SlopeScaledDepthBias = 0.0f;
    EnableMultisamplingDesc.DepthBiasClamp = 0.0f;
    EnableMultisamplingDesc.DepthClipEnable = TRUE;
    EnableMultisamplingDesc.ScissorEnable = FALSE;
    EnableMultisamplingDesc.MultisampleEnable = FALSE;
    EnableMultisamplingDesc.AntialiasedLineEnable = FALSE;
    EnableMultisamplingDesc.MultisampleEnable = TRUE;
    V(device->CreateRasterizerState(&EnableMultisamplingDesc, &EnableMultisampling));

    V(device->CreateVertexShader(Main_RenderVS_bytecode, sizeof(Main_RenderVS_bytecode), NULL, &RenderVS));
    V(device->CreatePixelShader(Main_RenderPS_bytecode, sizeof(Main_RenderPS_bytecode), NULL, &RenderPS));

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

    D3D11_SAMPLER_DESC AnisotropicSamplerDesc;
    AnisotropicSamplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    AnisotropicSamplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
    AnisotropicSamplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
    AnisotropicSamplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
    AnisotropicSamplerDesc.MinLOD = -FLT_MAX;
    AnisotropicSamplerDesc.MaxLOD = FLT_MAX;
    AnisotropicSamplerDesc.MipLODBias = 0.0f;
    AnisotropicSamplerDesc.MaxAnisotropy = 1;
    AnisotropicSamplerDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
    AnisotropicSamplerDesc.BorderColor[0] = 1.0f;
    AnisotropicSamplerDesc.BorderColor[1] = 1.0f;
    AnisotropicSamplerDesc.BorderColor[2] = 1.0f;
    AnisotropicSamplerDesc.BorderColor[3] = 1.0f;
    AnisotropicSamplerDesc.Filter = D3D11_FILTER_ANISOTROPIC;
    AnisotropicSamplerDesc.MaxAnisotropy = 16;
    V(device->CreateSamplerState(&AnisotropicSamplerDesc, &AnisotropicSampler));

    D3D11_SAMPLER_DESC ShadowSamplerDesc;
    ShadowSamplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    ShadowSamplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
    ShadowSamplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
    ShadowSamplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
    ShadowSamplerDesc.MinLOD = -FLT_MAX;
    ShadowSamplerDesc.MaxLOD = FLT_MAX;
    ShadowSamplerDesc.MipLODBias = 0.0f;
    ShadowSamplerDesc.MaxAnisotropy = 1;
    ShadowSamplerDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
    ShadowSamplerDesc.BorderColor[0] = 1.0f;
    ShadowSamplerDesc.BorderColor[1] = 1.0f;
    ShadowSamplerDesc.BorderColor[2] = 1.0f;
    ShadowSamplerDesc.BorderColor[3] = 1.0f;
    ShadowSamplerDesc.Filter = D3D11_FILTER_COMPARISON_MIN_MAG_MIP_POINT;
    ShadowSamplerDesc.ComparisonFunc = D3D11_COMPARISON_LESS;
    V(device->CreateSamplerState(&ShadowSamplerDesc, &ShadowSampler));

    specularAOSRV = l_specularAOSRV;
    beckmannSRV = l_beckmannSRV;
    irradianceSRV = l_irradianceSRV;

    const D3D11_INPUT_ELEMENT_DESC layout[] = {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0}};
    UINT numElements = sizeof(layout) / sizeof(D3D11_INPUT_ELEMENT_DESC);

    V(device->CreateInputLayout(layout, numElements, Main_RenderVS_bytecode, sizeof(Main_RenderVS_bytecode), &vertexLayout));
}

void releaseMainEffect()
{
    SAFE_RELEASE(vertexLayout);
    SAFE_RELEASE(ShadowSampler);
    SAFE_RELEASE(AnisotropicSampler);
    SAFE_RELEASE(LinearSampler);
    SAFE_RELEASE(PointSampler);
    SAFE_RELEASE(RenderPS);
    SAFE_RELEASE(RenderVS);
    SAFE_RELEASE(EnableMultisampling);
    SAFE_RELEASE(NoBlending);
    SAFE_RELEASE(EnableDepthDisableStencil);
    SAFE_RELEASE(CbufUpdatedPerObject);
    SAFE_RELEASE(CbufUpdatedPerFrame);
}

void mainEffect_setAmbient(float ambient)
{
    mainEffect_UpdatedPerObject.ambient = ambient;
}

void mainEffect_setSSSEnabled(bool sssEnabled)
{
    mainEffect_UpdatedPerObject.sssEnabled = sssEnabled ? 1.0f : -1.0f;
}

void mainEffect_setSSSWidth(float sssWidth)
{
    mainEffect_UpdatedPerObject.sssWidth = sssWidth;
}

void mainEffect_setTranslucency(float translucency)
{
    mainEffect_UpdatedPerObject.translucency = translucency;
}

void mainEffect_setSpecularIntensity(float specularIntensity)
{
    mainEffect_UpdatedPerObject.specularIntensity = specularIntensity;
}

void mainEffect_setSpecularRoughness(float specularRoughness)
{
    mainEffect_UpdatedPerObject.specularRoughness = specularRoughness;
}

void mainEffect_setSpecularFresnel(float specularFresnel)
{
    mainEffect_UpdatedPerObject.specularFresnel = specularFresnel;
}

void mainEffect_setBumpiness(float bumpiness)
{
    mainEffect_UpdatedPerObject.bumpiness = bumpiness;
}

float mainEffect_getAmbient()
{
    return mainEffect_UpdatedPerObject.ambient;
}

void mainPass(ID3D11DeviceContext *context, ID3D11RenderTargetView *mainRT, ID3D11RenderTargetView *depthRT, ID3D11RenderTargetView *velocityRT, ID3D11RenderTargetView *specularsRT, ID3D11DepthStencilView *depthStencil)
{
    // Calculate current view-projection matrix:
    DirectX::XMFLOAT4X4 currViewProj;
    DirectX::XMStoreFloat4x4(&currViewProj, DirectX::XMMatrixMultiply(DirectX::XMLoadFloat4x4(&camera.getViewMatrix()), DirectX::XMLoadFloat4x4(&camera.getProjectionMatrix())));

    // Variables setup:
    mainEffect_UpdatedPerFrame.cameraPosition = camera.getEyePosition();
    mainEffect_UpdatedPerFrame.currProj = camera.getProjectionMatrix();

    for (int i = 0; i < N_LIGHTS; i++)
    {
        DirectX::XMVECTOR t = DirectX::XMVectorSubtract(DirectX::XMLoadFloat3(&lights[i].camera.getLookAtPosition()), DirectX::XMLoadFloat3(&lights[i].camera.getEyePosition()));
        DirectX::XMFLOAT3 dir;
        DirectX::XMStoreFloat3(&dir, DirectX::XMVector3Normalize(t));

        DirectX::XMFLOAT3 pos = lights[i].camera.getEyePosition();
        mainEffect_UpdatedPerFrame.lights[i].viewProjection = ShadowMap::getViewProjectionTextureMatrix(lights[i].camera.getViewMatrix(), lights[i].camera.getProjectionMatrix());
        mainEffect_UpdatedPerFrame.lights[i].projection = lights[i].camera.getProjectionMatrix();
        mainEffect_UpdatedPerFrame.lights[i].position = pos;
        mainEffect_UpdatedPerFrame.lights[i].direction = dir;
        mainEffect_UpdatedPerFrame.lights[i].color_attenuation = DirectX::XMFLOAT4(lights[i].color.x, lights[i].color.y, lights[i].color.z, lights[i].attenuation);
        mainEffect_UpdatedPerFrame.lights[i].falloffStart = cos(0.5f * lights[i].fov);
        mainEffect_UpdatedPerFrame.lights[i].falloffWidth = lights[i].falloffWidth;
        mainEffect_UpdatedPerFrame.lights[i].farPlane = lights[i].farPlane;
        mainEffect_UpdatedPerFrame.lights[i].bias = lights[i].bias;
        ID3D11ShaderResourceView *shadowMapSRV = *lights[i].shadowMap;
        context->PSSetShaderResources(TEX_SHADOW_MAPS + i, 1, &shadowMapSRV);
    }

    D3D11_MAPPED_SUBRESOURCE mappedResource;
    context->Map(CbufUpdatedPerFrame, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
    memcpy(mappedResource.pData, &mainEffect_UpdatedPerFrame, sizeof(struct UpdatedPerFrame));
    context->Unmap(CbufUpdatedPerFrame, 0);

    // Render target setup:
    float clearColor[4] = {0.0f, 0.0f, 0.0f, 0.0f};
    context->ClearDepthStencilView(depthStencil, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0, 0);
    context->ClearRenderTargetView(mainRT, clearColor);
    context->ClearRenderTargetView(depthRT, clearColor);
    context->ClearRenderTargetView(specularsRT, clearColor);

    ID3D11RenderTargetView *rt[] = {mainRT, depthRT, velocityRT, specularsRT};
    context->OMSetRenderTargets(4, rt, depthStencil);

    // Heads rendering:
    context->IASetInputLayout(vertexLayout);

    context->PSSetShaderResources(TEX_SPECULARAO, 1, &specularAOSRV);
    context->PSSetShaderResources(TEX_BECKMANN, 1, &beckmannSRV);
    context->PSSetShaderResources(TEX_IRRADIANCE, 1, &irradianceSRV);

    UINT StencilRef = 1;

    context->VSSetConstantBuffers(CB_UPDATEDPERFRAME, 1U, &CbufUpdatedPerFrame);
    context->VSSetConstantBuffers(CB_UPDATEDPEROBJECT, 1U, &CbufUpdatedPerObject);
    context->PSSetConstantBuffers(CB_UPDATEDPERFRAME, 1U, &CbufUpdatedPerFrame);
    context->PSSetConstantBuffers(CB_UPDATEDPEROBJECT, 1U, &CbufUpdatedPerObject);
    context->PSSetSamplers(SAMP_POINT, 1, &PointSampler);
    context->PSSetSamplers(SAMP_LINEAR, 1, &LinearSampler);
    context->PSSetSamplers(SAMP_ANISOTROPIS, 1, &AnisotropicSampler);
    context->PSSetSamplers(SAMP_SHADOW, 1, &ShadowSampler);
    context->VSSetShader(RenderVS, NULL, 0);
    context->GSSetShader(NULL, NULL, 0);
    context->PSSetShader(RenderPS, NULL, 0);
    context->RSSetState(EnableMultisampling);
    context->OMSetDepthStencilState(EnableDepthDisableStencil, StencilRef);
    FLOAT BlendFactor[4] = {0.0f, 0.0f, 0.0f, 0.0f};
    context->OMSetBlendState(NoBlending, BlendFactor, 0xFFFFFFFF);

    for (int i = 0; i < N_HEADS; i++)
    {
        DirectX::XMFLOAT4X4 world;
        DirectX::XMStoreFloat4x4(&world, DirectX::XMMatrixTranslation(i - (N_HEADS - 1) / 2.0f, 0.0f, 0.0f));

        DirectX::XMFLOAT4X4 currWorldViewProj;
        DirectX::XMStoreFloat4x4(&currWorldViewProj, DirectX::XMMatrixMultiply(DirectX::XMLoadFloat4x4(&world), DirectX::XMLoadFloat4x4(&currViewProj)));

        mainEffect_UpdatedPerObject.currWorldViewProj = currWorldViewProj;
        mainEffect_UpdatedPerObject.world = world;
        DirectX::XMStoreFloat4x4(&mainEffect_UpdatedPerObject.worldInverseTranspose, DirectX::XMMatrixTranspose(DirectX::XMLoadFloat4x4(&world)));

        context->Map(CbufUpdatedPerObject, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
        memcpy(mappedResource.pData, &mainEffect_UpdatedPerObject, sizeof(struct UpdatedPerObject));
        context->Unmap(CbufUpdatedPerObject, 0);

        mesh.Render(context, TEX_DIFFUSE, TEX_NORMAL, TEX_SPECULAR);
    }

    ID3D11RenderTargetView *pRenderTargetViews[4] = {NULL, NULL, NULL, NULL};
    context->OMSetRenderTargets(4, pRenderTargetViews, NULL);

    ID3D11ShaderResourceView *pShaderResourceViews[16] = {NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL};
    context->PSSetShaderResources(0, 16, pShaderResourceViews);
}
