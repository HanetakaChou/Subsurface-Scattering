#ifndef _DEMO_H_
#define _DEMO_H_ 1

#include <sdkddkver.h>
#define NOMINMAX 1
#define WIN32_LEAN_AND_MEAN 1
#include <DXUT.h>
#include <SDKmesh.h>
#include "Camera.h"
#include "ShadowMap.h"

// Scene Data

extern Camera camera;

struct Light
{
	Camera camera;
	float fov;
	float falloffWidth;
	DirectX::XMFLOAT3 color;
	float attenuation;
	float farPlane;
	float bias;
	ShadowMap* shadowMap;
};

const int N_LIGHTS = 5;
const int N_HEADS = 1;

extern struct Light lights[N_LIGHTS];

extern CDXUTSDKMesh mesh;

#endif