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

#ifndef _CAMERA_H_
#define _CAMERA_H_ 1

#include <sdkddkver.h>
#define NOMINMAX 1
#define WIN32_LEAN_AND_MEAN 1
#include <Windows.h>
#include <DirectXMath.h>
#include <iostream>

class Camera
{
public:
	Camera() : distance(0.0f),
		distanceVelocity(0.0f),
		angle(0.0f, 0.0f),
		angularVelocity(0.0f, 0.0f),
		panPosition(0.0f, 0.0f),
		panVelocity(0.0f, 0.0f),
		viewportSize(1.0f, 1.0f),
		mousePos(0.0f, 0.0f),
		attenuation(0.0f),
		draggingLeft(false),
		draggingMiddle(false),
		draggingRight(false) {
		build();
	}

	LRESULT handleMessages(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);

	void frameMove(FLOAT elapsedTime);

	void setDistance(float var_distance) { this->distance = var_distance; }
	float getDistance() const { return distance; }

	void setDistanceVelocity(float var_distanceVelocity) { this->distanceVelocity = var_distanceVelocity; }
	float getDistanceVelocity() const { return distanceVelocity; }

	void setPanPosition(const DirectX::XMFLOAT2& var_panPosition) { this->panPosition = var_panPosition; }
	const DirectX::XMFLOAT2& getPanPosition() const { return panPosition; }

	void setPanVelocity(const DirectX::XMFLOAT2& var_panVelocity) { this->panVelocity = var_panVelocity; }
	const DirectX::XMFLOAT2& getPanVelocity() const { return panVelocity; }

	void setAngle(const DirectX::XMFLOAT2& var_angle) { this->angle = var_angle; }
	const DirectX::XMFLOAT2& getAngle() const { return angle; }

	void setAngularVelocity(const DirectX::XMFLOAT2& var_angularVelocity) { this->angularVelocity = var_angularVelocity; }
	const DirectX::XMFLOAT2& getAngularVelocity() const { return angularVelocity; }

	void setProjection(float fov, float aspect, float nearPlane, float farPlane);
	void setViewportSize(const DirectX::XMFLOAT2& var_viewportSize) { this->viewportSize = var_viewportSize; }

	const DirectX::XMFLOAT4X4& getViewMatrix() { return view; }
	const DirectX::XMFLOAT4X4& getProjectionMatrix() const { return projection; }

	const DirectX::XMFLOAT3& getLookAtPosition() { return lookAtPosition; }
	const DirectX::XMFLOAT3& getEyePosition() { return eyePosition; }

	friend std::ostream& operator<<(std::ostream& os, const Camera& camera);
	friend std::istream& operator>>(std::istream& is, Camera& camera);

private:
	void build();
	void updatePosition(DirectX::XMFLOAT2 delta);

	float distance;
	float distanceVelocity;
	DirectX::XMFLOAT2 panPosition;
	DirectX::XMFLOAT2 panVelocity;
	DirectX::XMFLOAT2 angle;
	DirectX::XMFLOAT2 angularVelocity;
	DirectX::XMFLOAT2 viewportSize;

	DirectX::XMFLOAT4X4 view, projection;
	DirectX::XMFLOAT3 lookAtPosition;
	DirectX::XMFLOAT3 eyePosition;

	DirectX::XMFLOAT2 mousePos;
	float attenuation;
	bool draggingLeft;
	bool draggingMiddle;
	bool draggingRight;
};

#endif
