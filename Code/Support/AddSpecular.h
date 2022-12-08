#ifndef _ADD_SPECULAR_H_
#define _ADD_SPECULAR_H_ 1

#include "DXUT.h"
#include "RenderTarget.h"

void initAddSpecular(ID3D11Device *device);
void releaseAddSpecular();
void addSpecular(ID3D11DeviceContext *context, ID3D11RenderTargetView *dst, ID3D11ShaderResourceView *specularsRT);

#endif