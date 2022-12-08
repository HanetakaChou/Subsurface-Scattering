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

#include "Camera.h"
using namespace std;

LRESULT Camera::handleMessages(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	switch (msg)
	{
	case WM_LBUTTONDOWN:
	{
		POINT point;
		GetCursorPos(&point);
		mousePos = DirectX::XMFLOAT2(float(point.x), float(point.y));
		draggingLeft = true;
		attenuation = 4.0f;
		SetCapture(hwnd);
		return true;
	}
	case WM_LBUTTONUP:
		draggingLeft = false;
		if (wparam & MK_CONTROL)
			attenuation = 0.0f;
		else
			attenuation = 4.0f;
		ReleaseCapture();
		return true;
	case WM_RBUTTONDOWN:
	{
		POINT point;
		GetCursorPos(&point);
		mousePos = DirectX::XMFLOAT2(float(point.x), float(point.y));
		draggingRight = true;
		SetCapture(hwnd);
		return true;
	}
	case WM_RBUTTONUP:
	{
		draggingRight = false;
		ReleaseCapture();
		return true;
	}
	case WM_MBUTTONDOWN:
	{
		POINT point;
		GetCursorPos(&point);
		mousePos = DirectX::XMFLOAT2(float(point.x), float(point.y));
		draggingMiddle = true;
		SetCapture(hwnd);
		return true;
	}
	case WM_MBUTTONUP:
	{
		draggingMiddle = false;
		ReleaseCapture();
		return true;
	}
	case WM_MOUSEMOVE:
	{
		POINT point;
		GetCursorPos(&point);
		DirectX::XMFLOAT2 newMousePos = DirectX::XMFLOAT2(float(point.x), float(point.y));
		if (draggingLeft)
		{
			DirectX::XMFLOAT2 delta = DirectX::XMFLOAT2(newMousePos.x - mousePos.x, newMousePos.y - mousePos.y);
			angularVelocity = DirectX::XMFLOAT2(angularVelocity.x - delta.x, angularVelocity.y - delta.y);
			mousePos = newMousePos;
		}
		if (draggingMiddle)
		{
			DirectX::XMFLOAT2 delta = DirectX::XMFLOAT2(newMousePos.x - mousePos.x, newMousePos.y - mousePos.y);
			updatePosition(delta);
			mousePos = newMousePos;
		}
		if (draggingRight)
		{
			distance += (newMousePos.y - mousePos.y) / 75.0f;
			mousePos = newMousePos;
		}
		return true;
	}
	case WM_MOUSEWHEEL:
	{
		short value = short(HIWORD(wparam));
		distance -= float(value) / 400.0f;
		return 0;
	}
	case WM_CAPTURECHANGED:
	{
		if ((HWND)lparam != hwnd)
		{
			draggingLeft = false;
			draggingMiddle = false;
			draggingRight = false;
		}
		break;
	}
	}
	return FALSE;
}

void Camera::frameMove(FLOAT elapsedTime)
{
	angle = DirectX::XMFLOAT2(angle.x + angularVelocity.x * elapsedTime / 150.0f, angle.y + angularVelocity.y * elapsedTime / 150.0f);
	angularVelocity = DirectX::XMFLOAT2(angularVelocity.x / (1.0f + attenuation * elapsedTime), angularVelocity.y / (1.0f + attenuation * elapsedTime));
	distance += distanceVelocity * elapsedTime / 150.0f;
	panPosition = DirectX::XMFLOAT2(panPosition.x + panVelocity.x * elapsedTime / 150.0f, panPosition.y + panVelocity.y * elapsedTime / 150.0f);
	build();
}

void Camera::setProjection(float fov, float aspect, float nearPlane, float farPlane)
{
	DirectX::XMStoreFloat4x4(&projection, DirectX::XMMatrixPerspectiveFovLH(fov, aspect, nearPlane, farPlane));
}

void Camera::build()
{
	DirectX::XMStoreFloat4x4(&view, DirectX::XMMatrixTranslation(-panPosition.x, -panPosition.y, distance));

	DirectX::XMMATRIX t = DirectX::XMMatrixRotationX(angle.y);
	DirectX::XMStoreFloat4x4(&view, DirectX::XMMatrixMultiply(t, DirectX::XMLoadFloat4x4(&view)));

	t = DirectX::XMMatrixRotationY(angle.x);
	DirectX::XMStoreFloat4x4(&view, DirectX::XMMatrixMultiply(t, DirectX::XMLoadFloat4x4(&view)));

	DirectX::XMMATRIX viewInverse = DirectX::XMMatrixInverse(NULL, DirectX::XMLoadFloat4x4(&view));

	DirectX::XMFLOAT4 lookAtPosition4 = DirectX::XMFLOAT4(0.0f, 0.0f, distance, 1.0f);
	DirectX::XMStoreFloat3(&lookAtPosition, DirectX::XMVector4Transform(DirectX::XMLoadFloat4(&lookAtPosition4), viewInverse));

	DirectX::XMFLOAT4 eyePosition4 = DirectX::XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);
	DirectX::XMStoreFloat3(&eyePosition, DirectX::XMVector4Transform(DirectX::XMLoadFloat4(&eyePosition4), viewInverse));
}

void Camera::updatePosition(DirectX::XMFLOAT2 delta)
{
	delta.x /= viewportSize.x / 2.0f;
	delta.y /= viewportSize.y / 2.0f;

	DirectX::XMMATRIX transform = DirectX::XMMatrixTranslation(0.0f, 0.0f, distance);
	transform = DirectX::XMMatrixMultiply(transform, DirectX::XMLoadFloat4x4(&projection));

	DirectX::XMMATRIX inverse = DirectX::XMMatrixInverse(NULL, transform);

	DirectX::XMFLOAT4 t = DirectX::XMFLOAT4(panPosition.x, panPosition.y, 0.0f, 1.0f);
	DirectX::XMStoreFloat4(&t, DirectX::XMVector4Transform(DirectX::XMLoadFloat4(&t), transform));
	t.x -= delta.x * t.w;
	t.y += delta.y * t.w;
	DirectX::XMStoreFloat4(&t, DirectX::XMVector4Transform(DirectX::XMLoadFloat4(&t), inverse));
	panPosition = DirectX::XMFLOAT2(t.x, t.y);
}

ostream& operator<<(ostream& os, const Camera& camera)
{
	os << camera.distance << endl;
	os << camera.angle.x << " " << camera.angle.y << endl;
	os << camera.panPosition.x << " " << camera.panPosition.y << endl;
	os << camera.angularVelocity.x << " " << camera.angularVelocity.y << endl;
	return os;
}

istream& operator>>(istream& is, Camera& camera)
{
	is >> camera.distance;
	is >> camera.angle.x >> camera.angle.y;
	is >> camera.panPosition.x >> camera.panPosition.y;
	is >> camera.angularVelocity.x >> camera.angularVelocity.y;
	return is;
}
