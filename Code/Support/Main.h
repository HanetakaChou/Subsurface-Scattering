#ifndef _MAIN_H_
#define _MAIN_H_ 1

#include <sdkddkver.h>
#define NOMINMAX 1
#define WIN32_LEAN_AND_MEAN 1
#include <DXUT.h>
#include "RenderTarget.h"
#include <DirectXMath.h>

void initMainEffect(ID3D11Device* device, ID3D11ShaderResourceView* specularAOSRV, ID3D11ShaderResourceView* irradianceSRV);
void releaseMainEffect();

void mainEffect_setAmbient(float ambient);
void mainEffect_setSpecularLightEnabled(bool specularlightEnabled);
void mainEffect_setSkyLightEnabled(bool skylightEnabled);
void mainEffect_setSSSEnabled(bool sssEnabled);
void mainEffect_setPostScatterEnabled(bool postscatterEnabled);
void mainEffect_setWorldScale(float worldScale);
void mainEffect_setScatteringDistance(DirectX::XMFLOAT3 scatteringDistance);
void mainEffect_setTransmittanceTint(DirectX::XMFLOAT3 transmittanceTint);
void mainEffect_setSpecularIntensity(float specularIntensity);
void mainEffect_setSpecularRoughness(float specularRoughness);
void mainEffect_setSpecularFresnel(float specularFresnel);
void mainEffect_setBumpiness(float bumpiness);

float mainEffect_getAmbient();

void mainPass(ID3D11DeviceContext* context, ID3D11RenderTargetView* mainRT, ID3D11RenderTargetView* depthRT, ID3D11RenderTargetView* albedoRT, ID3D11RenderTargetView* irradianceRT, ID3D11DepthStencilView* depthStencil);

#endif
