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

#include "DXUT.h"
#include "DXUTgui.h"
#include "Demo.h"

#include <fstream>
#include <sstream>
#include <iomanip>
#include <limits>

#include "Timer.h"
#include "Camera.h"
#include "RenderTarget.h"
#include "ShadowMap.h"
#include "SeparableSSS.h"
#include "FilmGrain.h"
#include "SkyDome.h"
#include "Main.h"
#include "AddSpecular.h"

using namespace std;

CDXUTDialogResourceManager dialogResourceManager;
CDXUTDialog mainHud;
CDXUTDialog secondaryHud;

Timer *timer;
CDXUTTextHelper *txtHelper;
ID3DUserDefinedAnnotation *d3dPerf;

CDXUTSDKMesh mesh;
ID3D11ShaderResourceView *specularAOSRV;
ID3D11ShaderResourceView *beckmannSRV;
ID3D11ShaderResourceView *irradianceSRV[3];

Camera camera;
SkyDome *skyDome[3];

int scene = 0;
float sceneTime = 0.0f;
int currentSkyDome = 0;

float lastTimeMeasured = 0.0f;
int nFrames = 0;
int nFramesInverval = 0;
int minFrames = numeric_limits<int>::max(), maxFrames = numeric_limits<int>::min();
float introTime = 0.0f;

BackbufferRenderTarget *backbufferRT;
RenderTarget *mainRTMS;
RenderTarget *mainRT;
RenderTarget *tmpRT_SRGB;
RenderTarget *tmpRT;
RenderTarget *finalRT[2];
RenderTarget *specularsRTMS;
RenderTarget *specularsRT;
RenderTarget *depthRTMS;
RenderTarget *depthRT;
RenderTarget *velocityRTMS;
RenderTarget *velocityRT;
DepthStencil *depthStencilMS;
DepthStencil *depthStencil;

SeparableSSS *separableSSS;
FilmGrain *filmGrain;

int subsampleIndex = 0;

struct MSAAMode
{
    wstring name;
    DXGI_SAMPLE_DESC desc;
};
MSAAMode msaaModes[] = {
    {L"MSAA 1x", {1, 0}},
    {L"MSAA 2x", {2, 0}},
    {L"MSAA 4x", {4, 0}},
    {L"CSAA 8x", {4, 8}},
    {L"CSAA 8xQ ", {8, 8}},
    {L"CSAA 16x", {4, 16}},
    {L"CSAA 16xQ", {8, 16}}};
vector<MSAAMode> supportedMsaaModes;
int antialiasingMode = 10;

bool showHud = true;
bool loaded = false;

const int SHADOW_MAP_SIZE = 2048;
const int HUD_WIDTH = 140;
const int HUD_WIDTH2 = 100;
const float CAMERA_FOV = 20.0f;

struct Light lights[N_LIGHTS];

enum Object
{
    OBJECT_CAMERA,
    OBJECT_LIGHT1,
    OBJECT_LIGHT2,
    OBJECT_LIGHT3,
    OBJECT_LIGHT4,
    OBJECT_LIGHT5
};
Object object = OBJECT_CAMERA;

enum State
{
    STATE_SPLASH_INTRO,
    STATE_INTRO,
    STATE_SPLASH_OUTRO,
    STATE_RUNNING
};

State state = STATE_RUNNING;

#define IDC_MSAA 5
#define IDC_HDR 6
#define IDC_PROFILE 8
#define IDC_SSS 9
#define IDC_NSAMPLES_LABEL 11
#define IDC_NSAMPLES 12
#define IDC_SSSWIDTH_LABEL 13
#define IDC_SSSWIDTH 14
#define IDC_STRENGTH_LABEL 15
#define IDC_STRENGTH_R_LABEL 16
#define IDC_STRENGTH_R 17
#define IDC_STRENGTH_G_LABEL 18
#define IDC_STRENGTH_G 19
#define IDC_STRENGTH_B_LABEL 20
#define IDC_STRENGTH_B 21
#define IDC_FALLOFF_LABEL 22
#define IDC_FALLOFF_R_LABEL 23
#define IDC_FALLOFF_R 24
#define IDC_FALLOFF_G_LABEL 25
#define IDC_FALLOFF_G 26
#define IDC_FALLOFF_B_LABEL 27
#define IDC_FALLOFF_B 28
#define IDC_TRANSLUCENCY_LABEL 29
#define IDC_TRANSLUCENCY 30
#define IDC_SPEC_INTENSITY_LABEL 31
#define IDC_SPEC_INTENSITY 32
#define IDC_SPEC_ROUGHNESS_LABEL 33
#define IDC_SPEC_ROUGHNESS 34
#define IDC_SPEC_FRESNEL_LABEL 35
#define IDC_SPEC_FRESNEL 36
#define IDC_BUMPINESS_LABEL 37
#define IDC_BUMPINESS 38
#define IDC_ENVMAP_LABEL 41
#define IDC_ENVMAP 42
#define IDC_AMBIENT 43
#define IDC_AMBIENT_LABEL 44
#define IDC_CAMERA_BUTTON 51
#define IDC_LIGHT1_BUTTON 52
#define IDC_LIGHT1_LABEL (53 + 5)
#define IDC_LIGHT1 (54 + 5 * 2)

MSAAMode currentMode()
{
    if (antialiasingMode < 10)
        return supportedMsaaModes[0];
    else
        return supportedMsaaModes[antialiasingMode - 10];
}

bool isMSAA()
{
    if (antialiasingMode < 10)
        return false;
    else
        return currentMode().desc.Count > 1;
}

void renderText()
{
    txtHelper->Begin();
    txtHelper->SetInsertionPos(7, 5);
    txtHelper->SetForegroundColor(DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f));

    wstringstream s;

    if (nFrames > 0)
    {
        s << setprecision(2) << std::fixed;
        s << "Min FPS: " << minFrames << endl;
        txtHelper->DrawTextLine(s.str().c_str());

        s.str(L"");
        s << "Max FPS: " << maxFrames << endl;
        txtHelper->DrawTextLine(s.str().c_str());

        s.str(L"");
        s << "Average FPS: " << nFrames / introTime << endl;
        txtHelper->DrawTextLine(s.str().c_str());

        txtHelper->DrawTextLine(L"");
    }

    if (timer->isEnabled())
    {
        s.str(L"");
        s << *timer;
        txtHelper->DrawTextLine(s.str().c_str());
    }

    txtHelper->End();
}

void renderScene(ID3D11DeviceContext *context, double, float)
{
    // Shadow Pass
    timer->start(context);
    d3dPerf->BeginEvent(L"Shadow Pass");
    shadowPass(context);
    d3dPerf->EndEvent();
    timer->clock(context, L"Shadows");

    // Main Pass
    d3dPerf->BeginEvent(L"Main Pass");
    if (isMSAA())
    {
        mainPass(context, *mainRTMS, *depthRTMS, *velocityRTMS, *specularsRTMS, *depthStencilMS);

        // Sky dome rendering:
        {
            context->OMSetRenderTargets(1, *mainRTMS, *depthStencilMS);

            skyDome[currentSkyDome]->render(context, camera.getViewMatrix(), camera.getProjectionMatrix(), 10.0f);

            ID3D11RenderTargetView *pRenderTargetViews[1] = {NULL};
            context->OMSetRenderTargets(1, pRenderTargetViews, NULL);
        }

        // Multisample buffers resolve:
        DXGI_FORMAT format = mainHud.GetCheckBox(IDC_HDR)->GetChecked() ? DXGI_FORMAT_R16G16B16A16_FLOAT : DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
        context->ResolveSubresource(*mainRT, 0, *mainRTMS, 0, format);
        context->ResolveSubresource(*depthRT, 0, *depthRTMS, 0, DXGI_FORMAT_R32_FLOAT);
        context->ResolveSubresource(*specularsRT, 0, *specularsRTMS, 0, format);
    }
    else
    {
        mainPass(context, *mainRT, *depthRT, *velocityRT, *specularsRT, *depthStencil);

        // Sky dome rendering:
        {
            context->OMSetRenderTargets(1, *mainRT, *depthStencil);

            skyDome[currentSkyDome]->render(context, camera.getViewMatrix(), camera.getProjectionMatrix(), 10.0f);

            ID3D11RenderTargetView *pRenderTargetViews[1] = {NULL};
            context->OMSetRenderTargets(1, pRenderTargetViews, NULL);
        }
    }
    d3dPerf->EndEvent();
    timer->clock(context, L"Main Pass");

    // Subsurface Scattering Pass
    d3dPerf->BeginEvent(L"separableSSS");
    if (mainHud.GetCheckBox(IDC_SSS)->GetChecked())
    {
        separableSSS->go(context, *mainRT, *mainRT, *depthRT, *depthStencil, *specularsRT);
    }
    d3dPerf->EndEvent();
    timer->clock(context, L"Skin");

    d3dPerf->BeginEvent(L"AddSpecular");
    addSpecular(context, *mainRT, *specularsRT);
    d3dPerf->EndEvent();
    timer->clock(context, L"Separate Speculars");

    // Film Grain Pass
    d3dPerf->BeginEvent(L"Film Grain");
    filmGrain->go(context, *mainRT, *backbufferRT);
    d3dPerf->EndEvent();
    timer->clock(context, L"Film Grain");
}

void updateLightControls(int i)
{
    wstringstream s;
    s << lights[i].color.x;
    secondaryHud.GetStatic(IDC_LIGHT1_LABEL + i)->SetText(s.str().c_str());
    secondaryHud.GetSlider(IDC_LIGHT1 + i)->SetValue(int((lights[i].color.x / 5.0f) * 100.0f));
}

float updateSlider(CDXUTDialog &hud, UINT slider, UINT label, float scale, wstring text)
{
    int min, max;
    hud.GetSlider(slider)->GetRange(min, max);
    float value = scale * float(hud.GetSlider(slider)->GetValue()) / (max - min);

    wstringstream s;
    s << text << value;
    hud.GetStatic(label)->SetText(s.str().c_str());

    return value;
}

void loadShot(istream &f)
{
    f >> camera;

    for (int i = 0; i < N_LIGHTS; i++)
    {
        f >> lights[i].camera;
        f >> lights[i].color.x;
        f >> lights[i].color.y;
        f >> lights[i].color.z;
        updateLightControls(i);
    }

    float ambient;
    f >> ambient;
    secondaryHud.GetSlider(IDC_AMBIENT)->SetValue(int(ambient * 100.0f));
    updateSlider(secondaryHud, IDC_AMBIENT, IDC_AMBIENT_LABEL, 1.0f, L"Ambient: ");
    mainEffect_setAmbient(ambient);

    float environment;
    f >> environment;
    secondaryHud.GetSlider(IDC_ENVMAP)->SetValue(int(environment * 100.0f));
    updateSlider(secondaryHud, IDC_ENVMAP, IDC_ENVMAP_LABEL, 1.0f, L"Enviroment: ");
    for (int i = 0; i < 3; i++)
        skyDome[i]->setIntensity(environment);

    float value;
    f >> value;

    f >> value;

    f >> value;
}

void loadPreset(int i)
{
    HRESULT hr;

    wstringstream s;
    s << L"Presets\\Preset" << i << L".txt";

    WCHAR strPath[512];
    V(DXUTFindDXSDKMediaFileCch(strPath, _countof(strPath), s.str().c_str()));

    ifstream f(strPath, ifstream::in);
    loadShot(f);
}

void correct(float amount)
{
    camera.setDistanceVelocity(amount * camera.getDistanceVelocity());
    camera.setPanVelocity(DirectX::XMFLOAT2(amount * camera.getPanVelocity().x, amount * camera.getPanVelocity().y));
    camera.setAngularVelocity(DirectX::XMFLOAT2(amount * camera.getAngularVelocity().x, amount * camera.getAngularVelocity().y));
}

void reset()
{
    loadPreset(9);
    camera.setDistanceVelocity(0.0f);
    camera.setPanVelocity(DirectX::XMFLOAT2(0.0f, 0.0f));

    mainEffect_setSpecularRoughness(0.3f);
    mainEffect_setSpecularIntensity(1.88f);
}

void CALLBACK onFrameRender(ID3D11Device *, ID3D11DeviceContext *context, double time, float elapsedTime, void *)
{
    switch (state)
    {
    case STATE_RUNNING:
    {
        // Render the scene:
        renderScene(context, time, elapsedTime);

        context->OMSetRenderTargets(1, *backbufferRT, NULL);
        // Render the HUD:
        d3dPerf->BeginEvent(L"HUD");
        if (showHud)
        {
            mainHud.OnRender(elapsedTime);
            secondaryHud.OnRender(elapsedTime);
            renderText();
        }
        d3dPerf->EndEvent();
        break;
    }
    }
}

void setupSSS(ID3D11Device *device, const DXGI_SURFACE_DESC *desc)
{
    int min, max;
    mainHud.GetSlider(IDC_SSSWIDTH)->GetRange(min, max);
    float sssLevel = 0.025f * float(mainHud.GetSlider(IDC_SSSWIDTH)->GetValue()) / (max - min);
    float value = 33.0f * float(mainHud.GetSlider(IDC_NSAMPLES)->GetValue()) / (max - min);
    int nSamples = std::max(3, 2 * (int(value) / 2) + 1);
    DXGI_FORMAT format = mainHud.GetCheckBox(IDC_HDR)->GetChecked() ? DXGI_FORMAT_R16G16B16A16_FLOAT : DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;

    SAFE_DELETE(separableSSS);
    separableSSS = new SeparableSSS(device, desc->Width, desc->Height, sssLevel, nSamples, format);

    DirectX::XMFLOAT3 strength;
    mainHud.GetSlider(IDC_STRENGTH_R)->GetRange(min, max);
    strength.x = float(mainHud.GetSlider(IDC_STRENGTH_R)->GetValue()) / (max - min);
    strength.y = float(mainHud.GetSlider(IDC_STRENGTH_G)->GetValue()) / (max - min);
    strength.z = float(mainHud.GetSlider(IDC_STRENGTH_B)->GetValue()) / (max - min);
    separableSSS->setStrength(strength);

    DirectX::XMFLOAT3 falloff;
    mainHud.GetSlider(IDC_FALLOFF_R)->GetRange(min, max);
    falloff.x = float(mainHud.GetSlider(IDC_FALLOFF_R)->GetValue()) / (max - min);
    falloff.y = float(mainHud.GetSlider(IDC_FALLOFF_G)->GetValue()) / (max - min);
    falloff.z = float(mainHud.GetSlider(IDC_FALLOFF_B)->GetValue()) / (max - min);
    separableSSS->setFalloff(falloff);
}

Camera *currentObject()
{
    switch (object)
    {
    case OBJECT_CAMERA:
        return &camera;
    default:
        return &lights[object - OBJECT_LIGHT1].camera;
    }
}

void CALLBACK onFrameMove(double, float elapsedTime, void *)
{
    camera.frameMove(elapsedTime);
    for (int i = 0; i < N_LIGHTS; i++)
        lights[i].camera.frameMove(elapsedTime);
}

void CALLBACK keyboardProc(UINT nChar, bool keydown, bool, void *)
{
    if (keydown)
        switch (nChar)
        {
        case VK_TAB:
            showHud = !showHud;
            break;
        case 'O':
        {
            fstream f("Config.txt", fstream::out);
            f << camera;
            for (int i = 0; i < N_LIGHTS; i++)
            {
                f << lights[i].camera;
                f << lights[i].color.x << endl;
                f << lights[i].color.y << endl;
                f << lights[i].color.z << endl;
            }
            float ambient = mainEffect_getAmbient();
            f << ambient << endl;
            f << skyDome[currentSkyDome]->getIntensity() << endl;
            f << 0.0f << endl;
            f << 0.0f << endl;
            f << 0.0f << endl;
            break;
        }
        case 'P':
        {
            fstream f("Config.txt", fstream::in);
            loadShot(f);
            break;
        }
        case 'S':
        {
            bool sssEnabled = mainHud.GetCheckBox(IDC_SSS)->GetChecked();
            mainEffect_setSSSEnabled(!sssEnabled);
            break;
        }
        case 'X':
            mainHud.GetCheckBox(IDC_PROFILE)->SetChecked(!mainHud.GetCheckBox(IDC_PROFILE)->GetChecked());
            timer->setEnabled(mainHud.GetCheckBox(IDC_PROFILE)->GetChecked());
            break;
        case '1':
            object = OBJECT_CAMERA;
            for (int i = 0; i < N_LIGHTS + 1; i++)
                secondaryHud.GetButton(IDC_CAMERA_BUTTON + i)->SetEnabled(i != int(object));
            break;
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
            object = Object(OBJECT_LIGHT1 + nChar - '2');
            for (int i = 0; i < N_LIGHTS + 1; i++)
                secondaryHud.GetButton(IDC_CAMERA_BUTTON + i)->SetEnabled(i != int(object));
            break;
        case 'C':
        {
            if (DXUTIsKeyDown(VK_LCONTROL) || DXUTIsKeyDown(VK_RCONTROL))
            {
                string code = separableSSS->getKernelCode();
                if (OpenClipboard(DXUTGetHWND()))
                {
                    EmptyClipboard();
                    HGLOBAL data;
                    data = GlobalAlloc(GMEM_DDESHARE, code.size() + 1);
                    char *buffer = (char *)GlobalLock(data);
                    strcpy_s(buffer, code.size() + 1, code.c_str());
                    GlobalUnlock(data);
                    SetClipboardData(CF_TEXT, data);
                    CloseClipboard();
                }
            }
            break;
        }
        }
}

LRESULT CALLBACK msgProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam, bool *pbNoFurtherProcessing, void *)
{
    switch (state)
    {
    case STATE_RUNNING:
        *pbNoFurtherProcessing = dialogResourceManager.MsgProc(hwnd, msg, wparam, lparam);
        if (*pbNoFurtherProcessing)
            return 0;

        if (showHud)
        {
            *pbNoFurtherProcessing = mainHud.MsgProc(hwnd, msg, wparam, lparam);
            if (*pbNoFurtherProcessing)
                return 0;

            *pbNoFurtherProcessing = secondaryHud.MsgProc(hwnd, msg, wparam, lparam);
            if (*pbNoFurtherProcessing)
                return 0;
        }

        currentObject()->handleMessages(hwnd, msg, wparam, lparam);

        break;
    }

    return 0;
}

HRESULT CALLBACK onResizedSwapChain(ID3D11Device *device, IDXGISwapChain *swapChain, const DXGI_SURFACE_DESC *desc, void *)
{
    HRESULT hr;

    V_RETURN(dialogResourceManager.OnD3D11ResizedSwapChain(device, desc));

    DXGI_FORMAT format = mainHud.GetCheckBox(IDC_HDR)->GetChecked() ? DXGI_FORMAT_R16G16B16A16_FLOAT : DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;

    mainHud.GetComboBox(IDC_MSAA)->RemoveAllItems();

    supportedMsaaModes.clear();
    for (char i = 0; i < sizeof(msaaModes) / sizeof(MSAAMode); i++)
    {
        UINT quality;
        device->CheckMultisampleQualityLevels(format, msaaModes[i].desc.Count, &quality);
        if (quality > msaaModes[i].desc.Quality)
        {
            mainHud.GetComboBox(IDC_MSAA)->AddItem(msaaModes[i].name.c_str(), reinterpret_cast<void *>(static_cast<intptr_t>((i + 10))));
            supportedMsaaModes.push_back(msaaModes[i]);
        }
    }
    mainHud.GetComboBox(IDC_MSAA)->SetSelectedByData(reinterpret_cast<void *>(static_cast<intptr_t>(antialiasingMode)));

    if (isMSAA())
    {
        MSAAMode mode = currentMode();
        mainRTMS = new RenderTarget(device, desc->Width, desc->Height, format, mode.desc);
        depthRTMS = new RenderTarget(device, desc->Width, desc->Height, DXGI_FORMAT_R32_FLOAT, mode.desc);
        specularsRTMS = new RenderTarget(device, desc->Width, desc->Height, format, mode.desc);
        velocityRTMS = new RenderTarget(device, desc->Width, desc->Height, DXGI_FORMAT_R16G16_FLOAT, mode.desc);
        depthStencilMS = new DepthStencil(device, desc->Width, desc->Height, DXGI_FORMAT_R24G8_TYPELESS, DXGI_FORMAT_D24_UNORM_S8_UINT, DXGI_FORMAT_R24_UNORM_X8_TYPELESS, mode.desc);
    }

    backbufferRT = new BackbufferRenderTarget(device, swapChain);
    mainRT = new RenderTarget(device, desc->Width, desc->Height, format);
    tmpRT_SRGB = new RenderTarget(device, desc->Width, desc->Height, DXGI_FORMAT_R8G8B8A8_UNORM_SRGB);
    tmpRT = new RenderTarget(device, *tmpRT_SRGB, DXGI_FORMAT_R8G8B8A8_UNORM);
    for (int i = 0; i < 2; i++)
        finalRT[i] = new RenderTarget(device, desc->Width, desc->Height, DXGI_FORMAT_R8G8B8A8_UNORM_SRGB);

    depthRT = new RenderTarget(device, desc->Width, desc->Height, DXGI_FORMAT_R32_FLOAT);
    specularsRT = new RenderTarget(device, desc->Width, desc->Height, format);
    velocityRT = new RenderTarget(device, desc->Width, desc->Height, DXGI_FORMAT_R16G16_FLOAT);
    depthStencil = new DepthStencil(device, desc->Width, desc->Height, DXGI_FORMAT_R24G8_TYPELESS, DXGI_FORMAT_D24_UNORM_S8_UINT, DXGI_FORMAT_R24_UNORM_X8_TYPELESS);

    float aspect = (float)desc->Width / desc->Height;
    camera.setProjection(CAMERA_FOV * DirectX::XM_PI / 180.f, aspect, 0.1f, 100.0f);

    setupSSS(device, desc);

    filmGrain = new FilmGrain(device, desc->Width, desc->Height);

    camera.setViewportSize(DirectX::XMFLOAT2((float)desc->Width, (float)desc->Height));
    for (int i = 0; i < N_LIGHTS; i++)
        lights[i].camera.setViewportSize(DirectX::XMFLOAT2((float)desc->Width, (float)desc->Height));

    mainHud.SetLocation(desc->Width - (45 + HUD_WIDTH), 0);
    secondaryHud.SetLocation(0, desc->Height - 520);

    if (!loaded)
    {
        loadPreset(9);
        loaded = true;
    }

    return S_OK;
}

void CALLBACK onReleasingSwapChain(void *)
{
    dialogResourceManager.OnD3D11ReleasingSwapChain();

    SAFE_DELETE(mainRTMS);
    SAFE_DELETE(depthStencilMS);
    SAFE_DELETE(depthRTMS);
    SAFE_DELETE(specularsRTMS);
    SAFE_DELETE(velocityRTMS);

    SAFE_DELETE(backbufferRT);
    SAFE_DELETE(mainRT);
    SAFE_DELETE(tmpRT_SRGB);
    SAFE_DELETE(tmpRT);
    for (int i = 0; i < 2; i++)
        SAFE_DELETE(finalRT[i]);

    SAFE_DELETE(depthRT);
    SAFE_DELETE(specularsRT);
    SAFE_DELETE(velocityRT);
    SAFE_DELETE(depthStencil);

    SAFE_DELETE(separableSSS);
    SAFE_DELETE(filmGrain);
}

void CALLBACK onGUIEvent(UINT event, int id, CDXUTControl *control, void *)
{
    switch (id)
    {
    case IDC_MSAA:
    {
        CDXUTComboBox *box = (CDXUTComboBox *)control;
        antialiasingMode = static_cast<int>(reinterpret_cast<intptr_t>(box->GetSelectedData()));
        onReleasingSwapChain(NULL);
        onResizedSwapChain(DXUTGetD3D11Device(), DXUTGetDXGISwapChain(), DXUTGetDXGIBackBufferSurfaceDesc(), NULL);
        break;
    }
    case IDC_HDR:
        if (event == EVENT_CHECKBOX_CHANGED)
        {
            onReleasingSwapChain(NULL);
            onResizedSwapChain(DXUTGetD3D11Device(), DXUTGetDXGISwapChain(), DXUTGetDXGIBackBufferSurfaceDesc(), NULL);
        }
        break;
    case IDC_PROFILE:
        if (event == EVENT_CHECKBOX_CHANGED)
            timer->setEnabled(mainHud.GetCheckBox(IDC_PROFILE)->GetChecked());
        break;
    case IDC_SSS:
    {
        bool sssEnabled = mainHud.GetCheckBox(IDC_SSS)->GetChecked();
        mainEffect_setSSSEnabled(sssEnabled);
        break;
    }
    case IDC_NSAMPLES:
    {
        int min, max;
        mainHud.GetSlider(IDC_NSAMPLES)->GetRange(min, max);
        float value = 33.0f * float(mainHud.GetSlider(IDC_NSAMPLES)->GetValue()) / (max - min);
        int nSamples = std::max(3, 2 * (int(value) / 2) + 1);

        wstringstream s;
        s << L"Samples: " << nSamples;
        mainHud.GetStatic(IDC_NSAMPLES_LABEL)->SetText(s.str().c_str());

        separableSSS->setNSamples(nSamples);
        break;
    }
    case IDC_SSSWIDTH:
    {
        float value = updateSlider(mainHud, IDC_SSSWIDTH, IDC_SSSWIDTH_LABEL, 0.025f, L"SSS Width: ");
        separableSSS->setWidth(value);
        mainEffect_setSSSWidth(value);
        break;
    }
    case IDC_STRENGTH_R:
    case IDC_STRENGTH_G:
    case IDC_STRENGTH_B:
    {
        DirectX::XMFLOAT3 strength;
        strength.x = updateSlider(mainHud, IDC_STRENGTH_R, IDC_STRENGTH_R_LABEL, 1.0f, L"R: ");
        strength.y = updateSlider(mainHud, IDC_STRENGTH_G, IDC_STRENGTH_G_LABEL, 1.0f, L"G: ");
        strength.z = updateSlider(mainHud, IDC_STRENGTH_B, IDC_STRENGTH_B_LABEL, 1.0f, L"B: ");
        separableSSS->setStrength(strength);
        break;
    }
    case IDC_FALLOFF_R:
    case IDC_FALLOFF_G:
    case IDC_FALLOFF_B:
    {
        DirectX::XMFLOAT3 falloff;
        falloff.x = updateSlider(mainHud, IDC_FALLOFF_R, IDC_FALLOFF_R_LABEL, 1.0f, L"R: ");
        falloff.y = updateSlider(mainHud, IDC_FALLOFF_G, IDC_FALLOFF_G_LABEL, 1.0f, L"G: ");
        falloff.z = updateSlider(mainHud, IDC_FALLOFF_B, IDC_FALLOFF_B_LABEL, 1.0f, L"B: ");
        separableSSS->setFalloff(falloff);
        break;
    }
    case IDC_TRANSLUCENCY:
    {
        float value = updateSlider(mainHud, IDC_TRANSLUCENCY, IDC_TRANSLUCENCY_LABEL, 1.0f, L"Translucency: ");
        mainEffect_setTranslucency(value);
        break;
    }
    case IDC_SPEC_INTENSITY:
    {
        float value = updateSlider(secondaryHud, IDC_SPEC_INTENSITY, IDC_SPEC_INTENSITY_LABEL, 4.0f, L"Spec. Intensity: ");
        mainEffect_setSpecularIntensity(value);
        break;
    }
    case IDC_SPEC_ROUGHNESS:
    {
        float value = updateSlider(secondaryHud, IDC_SPEC_ROUGHNESS, IDC_SPEC_ROUGHNESS_LABEL, 1.0f, L"Spec. Rough.: ");
        mainEffect_setSpecularRoughness(value);
        break;
    }
    case IDC_SPEC_FRESNEL:
    {
        float value = updateSlider(secondaryHud, IDC_SPEC_FRESNEL, IDC_SPEC_FRESNEL_LABEL, 1.0f, L"Spec. Fresnel: ");
        mainEffect_setSpecularFresnel(value);
        break;
    }
    case IDC_BUMPINESS:
    {
        float value = updateSlider(secondaryHud, IDC_BUMPINESS, IDC_BUMPINESS_LABEL, 1.0f, L"Bumpiness: ");
        mainEffect_setBumpiness(value);
        break;
    }
    case IDC_ENVMAP:
    {
        float value = updateSlider(secondaryHud, IDC_ENVMAP, IDC_ENVMAP_LABEL, 1.0f, L"Enviroment: ");
        for (int i = 0; i < 3; i++)
            skyDome[i]->setIntensity(value);
        break;
    }
    case IDC_AMBIENT:
    {
        float value = updateSlider(secondaryHud, IDC_AMBIENT, IDC_AMBIENT_LABEL, 1.0f, L"Ambient: ");
        mainEffect_setAmbient(value);
        break;
    }
    case IDC_CAMERA_BUTTON:
    case IDC_LIGHT1_BUTTON:
    case IDC_LIGHT1_BUTTON + 1:
    case IDC_LIGHT1_BUTTON + 2:
    case IDC_LIGHT1_BUTTON + 3:
    case IDC_LIGHT1_BUTTON + 4:
        object = Object(id - IDC_CAMERA_BUTTON);
        for (int i = 0; i < N_LIGHTS + 1; i++)
            secondaryHud.GetButton(IDC_CAMERA_BUTTON + i)->SetEnabled(i != int(object));
        break;
    case IDC_LIGHT1:
    case IDC_LIGHT1 + 1:
    case IDC_LIGHT1 + 2:
    case IDC_LIGHT1 + 3:
    case IDC_LIGHT1 + 4:
    {
        float value = updateSlider(secondaryHud, id, IDC_LIGHT1_LABEL + id - IDC_LIGHT1, 5.0f, L"");
        lights[id - IDC_LIGHT1].color = DirectX::XMFLOAT3(value, value, value);
        break;
    }
    }
}

void loadMesh(CDXUTSDKMesh &var_mesh, ID3D11Device *device, const wstring &name, const wstring &)
{
    HRESULT hr;

    WCHAR strPath[512];
    V(DXUTFindDXSDKMediaFileCch(strPath, _countof(strPath), name.c_str()));
    V(var_mesh.Create(device, strPath, NULL));
}

HRESULT CALLBACK onCreateDevice(ID3D11Device *device, const DXGI_SURFACE_DESC *, void *)
{
    HRESULT hr;

    ID3D11DeviceContext *context = DXUTGetD3D11DeviceContext();
    V_RETURN(context->QueryInterface(IID_PPV_ARGS(&d3dPerf)));

    V_RETURN(dialogResourceManager.OnD3D11CreateDevice(device, context));

    timer = new Timer(device, context);
    timer->setEnabled(mainHud.GetCheckBox(IDC_PROFILE)->GetChecked());

    txtHelper = new CDXUTTextHelper(device, context, &dialogResourceManager, 15);

    loadMesh(mesh, device, L"Head\\Head.sdkmesh", L"Head");

    WCHAR strPath[512];
    V_RETURN(DXUTFindDXSDKMediaFileCch(strPath, _countof(strPath), L"Head\\SpecularAOMap.dds"));
    V_RETURN(DXUTGetGlobalResourceCache().CreateTextureFromFile(device, context, strPath, &specularAOSRV));

    V_RETURN(DXUTFindDXSDKMediaFileCch(strPath, _countof(strPath), L"BeckmannMap.dds"));
    V_RETURN(DXUTGetGlobalResourceCache().CreateTextureFromFile(device, context, strPath, &beckmannSRV));

    V_RETURN(DXUTFindDXSDKMediaFileCch(strPath, _countof(strPath), L"Enviroment\\StPeters\\IrradianceMap.dds"));
    V_RETURN(DXUTGetGlobalResourceCache().CreateTextureFromFile(device, context, strPath, &irradianceSRV[0]));

    V_RETURN(DXUTFindDXSDKMediaFileCch(strPath, _countof(strPath), L"Enviroment\\Grace\\IrradianceMap.dds"));
    V_RETURN(DXUTGetGlobalResourceCache().CreateTextureFromFile(device, context, strPath, &irradianceSRV[1]));

    V_RETURN(DXUTFindDXSDKMediaFileCch(strPath, _countof(strPath), L"Enviroment\\Eucalyptus\\IrradianceMap.dds"));
    V_RETURN(DXUTGetGlobalResourceCache().CreateTextureFromFile(device, context, strPath, &irradianceSRV[2]));

    initMainEffect(device, specularAOSRV, beckmannSRV, irradianceSRV[currentSkyDome]);
    initAddSpecular(device);

    int min, max;
    mainHud.GetSlider(IDC_TRANSLUCENCY)->GetRange(min, max);
    float scale = float(mainHud.GetSlider(IDC_TRANSLUCENCY)->GetValue()) / (max - min);
    mainEffect_setTranslucency(scale);

    bool sssEnabled = mainHud.GetCheckBox(IDC_SSS)->GetChecked();
    mainEffect_setSSSEnabled(sssEnabled);

    mainHud.GetSlider(IDC_TRANSLUCENCY)->GetRange(min, max);
    float sssWidth = 0.025f * float(mainHud.GetSlider(IDC_SSSWIDTH)->GetValue()) / (max - min);
    mainEffect_setSSSWidth(sssWidth);

    secondaryHud.GetSlider(IDC_SPEC_INTENSITY)->GetRange(min, max);
    float specularIntensity = float(secondaryHud.GetSlider(IDC_SPEC_INTENSITY)->GetValue()) / (max - min);
    mainEffect_setSpecularIntensity(specularIntensity);

    secondaryHud.GetSlider(IDC_SPEC_ROUGHNESS)->GetRange(min, max);
    float specularRoughness = float(secondaryHud.GetSlider(IDC_SPEC_ROUGHNESS)->GetValue()) / (max - min);
    mainEffect_setSpecularRoughness(specularRoughness);

    secondaryHud.GetSlider(IDC_SPEC_FRESNEL)->GetRange(min, max);
    float specularFresnel = float(secondaryHud.GetSlider(IDC_SPEC_FRESNEL)->GetValue()) / (max - min);
    mainEffect_setSpecularFresnel(specularFresnel);

    secondaryHud.GetSlider(IDC_BUMPINESS)->GetRange(min, max);
    float bumpiness = float(secondaryHud.GetSlider(IDC_BUMPINESS)->GetValue()) / (max - min);
    mainEffect_setBumpiness(bumpiness);

    secondaryHud.GetSlider(IDC_AMBIENT)->GetRange(min, max);
    float ambient = float(secondaryHud.GetSlider(IDC_AMBIENT)->GetValue()) / (max - min);
    mainEffect_setAmbient(ambient);

    for (int i = 0; i < N_LIGHTS; i++)
    {
        lights[i].fov = 45.0f * DirectX::XM_PI / 180.f;
        lights[i].falloffWidth = 0.05f;
        lights[i].attenuation = 1.0f / 128.0f;
        lights[i].farPlane = 10.0f;
        lights[i].bias = -0.01f;

        lights[i].camera.setDistance(2.0);
        lights[i].camera.setProjection(lights[i].fov, 1.0f, 0.1f, lights[i].farPlane);
        lights[i].color = DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f);
        lights[i].shadowMap = new ShadowMap(device, SHADOW_MAP_SIZE, SHADOW_MAP_SIZE);
    }

    skyDome[0] = new SkyDome(device, L"Enviroment\\StPeters", 0.0f);
    skyDome[1] = new SkyDome(device, L"Enviroment\\Grace", 0.0f);
    skyDome[2] = new SkyDome(device, L"Enviroment\\Eucalyptus", 0.0f);

    ShadowMap::init(device);

    return S_OK;
}

void CALLBACK onDestroyDevice(void *)
{
    d3dPerf->Release();

    dialogResourceManager.OnD3D11DestroyDevice();
    DXUTGetGlobalResourceCache().OnDestroyDevice();

    SAFE_DELETE(timer);
    SAFE_DELETE(txtHelper);

    mesh.Destroy();
    SAFE_RELEASE(specularAOSRV);
    SAFE_RELEASE(beckmannSRV);

    for (int i = 0; i < N_LIGHTS; i++)
        SAFE_DELETE(lights[i].shadowMap);

    for (int i = 0; i < 3; i++)
    {
        SAFE_DELETE(skyDome[i]);
        SAFE_RELEASE(irradianceSRV[i]);
    }

    releaseMainEffect();
    releaseAddSpecular();
    ShadowMap::release();
}

void initApp()
{
    mainHud.Init(&dialogResourceManager);
    mainHud.SetCallback(onGUIEvent);

    secondaryHud.Init(&dialogResourceManager);
    secondaryHud.SetCallback(onGUIEvent);

    /**
     * Create main hud (the one on the right)
     */
    int iY = 10;

    iY += 15;
    mainHud.AddComboBox(IDC_MSAA, 35, iY += 24, HUD_WIDTH, 22);

    iY += 15;
    mainHud.AddCheckBox(IDC_HDR, L"HDR", 35, iY += 24, HUD_WIDTH, 22, true);
    mainHud.AddCheckBox(IDC_PROFILE, L"Profile", 35, iY += 24, HUD_WIDTH, 22, false);

    iY += 15;
    mainHud.AddCheckBox(IDC_SSS, L"SSS Rendering", 35, iY += 24, HUD_WIDTH, 22, true);

    iY += 15;
    mainHud.AddStatic(IDC_NSAMPLES_LABEL, L"Samples: 17", 35, iY += 24, HUD_WIDTH, 22);
    mainHud.AddSlider(IDC_NSAMPLES, 35, iY += 24, HUD_WIDTH, 22, 0, 100, int((17.0f / 33.0f) * 100.0f));
    mainHud.AddStatic(IDC_SSSWIDTH_LABEL, L"SSS Width: 0.012", 35, iY += 24, HUD_WIDTH, 22);
    mainHud.AddSlider(IDC_SSSWIDTH, 35, iY += 24, HUD_WIDTH, 22, 0, 100, int((0.012f / 0.025f) * 100.0f));

    iY += 15;
    mainHud.AddStatic(IDC_STRENGTH_LABEL, L"SSS Strength", 35, iY += 24, HUD_WIDTH, 22);
    mainHud.AddStatic(IDC_STRENGTH_R_LABEL, L"R: 0.48", 35, iY += 24, 50, 22);
    mainHud.AddSlider(IDC_STRENGTH_R, 80, iY, HUD_WIDTH - 45, 22, 0, 100, int(0.48f * 100.0f));
    mainHud.AddStatic(IDC_STRENGTH_G_LABEL, L"G: 0.41", 35, iY += 24, 50, 22);
    mainHud.AddSlider(IDC_STRENGTH_G, 80, iY, HUD_WIDTH - 45, 22, 0, 100, int(0.41f * 100.0f));
    mainHud.AddStatic(IDC_STRENGTH_B_LABEL, L"B: 0.28", 35, iY += 24, 50, 22);
    mainHud.AddSlider(IDC_STRENGTH_B, 80, iY, HUD_WIDTH - 45, 22, 0, 100, int(0.28f * 100.0f));

    iY += 15;
    mainHud.AddStatic(IDC_FALLOFF_LABEL, L"SSS Falloff", 35, iY += 24, HUD_WIDTH, 22);
    mainHud.AddStatic(IDC_FALLOFF_R_LABEL, L"R: 1.0", 35, iY += 24, 50, 22);
    mainHud.AddSlider(IDC_FALLOFF_R, 80, iY, HUD_WIDTH - 45, 22, 0, 100, int(1.0f * 100.0f));
    mainHud.AddStatic(IDC_FALLOFF_G_LABEL, L"G: 0.37", 35, iY += 24, 50, 22);
    mainHud.AddSlider(IDC_FALLOFF_G, 80, iY, HUD_WIDTH - 45, 22, 0, 100, int(0.37f * 100.0f));
    mainHud.AddStatic(IDC_FALLOFF_B_LABEL, L"B: 0.3", 35, iY += 24, 50, 22);
    mainHud.AddSlider(IDC_FALLOFF_B, 80, iY, HUD_WIDTH - 45, 22, 0, 100, int(0.3f * 100.0f));

    iY += 15;
    mainHud.AddStatic(IDC_TRANSLUCENCY_LABEL, L"Translucency: 0.83", 35, iY += 24, HUD_WIDTH, 22);
    mainHud.AddSlider(IDC_TRANSLUCENCY, 35, iY += 24, HUD_WIDTH, 22, 0, 100, int(0.83f * 100.0f));

    /**
     * Create the speculars and light step hud (the one on the left)
     */
    iY = 0;
    int iX = 10;

    secondaryHud.AddStatic(IDC_SPEC_INTENSITY_LABEL, L"Spec. Intensity: 1.88", iX, iY, HUD_WIDTH2, 22);
    secondaryHud.AddSlider(IDC_SPEC_INTENSITY, iX, iY += 24, HUD_WIDTH2, 22, 0, 100, int((1.88f / 4.0f) * 100.0f));
    secondaryHud.AddStatic(IDC_SPEC_ROUGHNESS_LABEL, L"Spec. Rough.: 0.3", iX, iY += 24, HUD_WIDTH2, 22);
    secondaryHud.AddSlider(IDC_SPEC_ROUGHNESS, iX, iY += 24, HUD_WIDTH2, 22, 0, 100, int(0.3f * 100.0f));
    secondaryHud.AddStatic(IDC_SPEC_FRESNEL_LABEL, L"Spec. Fresnel: 0.82", iX, iY += 24, HUD_WIDTH2, 22);
    secondaryHud.AddSlider(IDC_SPEC_FRESNEL, iX, iY += 24, HUD_WIDTH2, 22, 0, 100, int(0.82f * 100.0f));
    iY += 15;

    secondaryHud.AddStatic(IDC_BUMPINESS_LABEL, L"Bumpiness: 0.9", iX, iY += 24, HUD_WIDTH2, 22);
    secondaryHud.AddSlider(IDC_BUMPINESS, iX, iY += 24, HUD_WIDTH2, 22, 0, 100, int(0.9f * 100.0f));
    iY += 15;

    secondaryHud.AddStatic(IDC_ENVMAP_LABEL, L"Env. Map: 0", iX, iY += 24, HUD_WIDTH2, 22);
    secondaryHud.AddSlider(IDC_ENVMAP, iX, iY += 24, HUD_WIDTH2, 22, 0, 100, int(0.0f * 100.0f));

    secondaryHud.AddStatic(IDC_AMBIENT_LABEL, L"Ambient: 0", iX, iY += 24, HUD_WIDTH2, 22);
    secondaryHud.AddSlider(IDC_AMBIENT, iX, iY += 24, HUD_WIDTH2, 22, 0, 100, int(0.0f * 100.0f));

    int top = iY - 24 * 3;
    iX += HUD_WIDTH2 + 20;

    iY = top;
    secondaryHud.AddButton(IDC_CAMERA_BUTTON, L"Camera", iX, iY += 24, 45, 22);
    secondaryHud.GetButton(IDC_CAMERA_BUTTON)->SetEnabled(false);
    iX += 45 + 20;

    for (int i = 0; i < N_LIGHTS; i++)
    {
        iY = top;

        wstringstream s;
        s << L"Light " << (i + 1);
        secondaryHud.AddButton(IDC_LIGHT1_BUTTON + i, s.str().c_str(), iX, iY += 24, 45, 22);
        secondaryHud.AddStatic(IDC_LIGHT1_LABEL + i, L"0", iX, iY += 24, 45, 22);
        secondaryHud.AddSlider(IDC_LIGHT1 + i, iX, iY += 24, 45, 22, 0, 100, 0);

        iX += 45 + 20;
    }
}

bool CALLBACK modifyDeviceSettings(DXUTDeviceSettings *settings, void *)
{
    settings->d3d11.AutoCreateDepthStencil = false;
    settings->d3d11.sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT | DXGI_USAGE_SHADER_INPUT;
    return true;
}

bool CALLBACK isDeviceAcceptable(const CD3D11EnumAdapterInfo *, UINT, const CD3D11EnumDeviceInfo *, DXGI_FORMAT, bool, void *)
{
    return true;
}

INT WINAPI wWinMain(HINSTANCE, HINSTANCE, LPWSTR, int)
{
// Enable run-time memory check for debug builds.
#if defined(DEBUG) | defined(_DEBUG)
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

    DXUTSetCallbackD3D11DeviceAcceptable(isDeviceAcceptable);
    DXUTSetCallbackD3D11DeviceCreated(onCreateDevice);
    DXUTSetCallbackD3D11DeviceDestroyed(onDestroyDevice);
    DXUTSetCallbackD3D11SwapChainResized(onResizedSwapChain);
    DXUTSetCallbackD3D11SwapChainReleasing(onReleasingSwapChain);
    DXUTSetCallbackD3D11FrameRender(onFrameRender);

    DXUTSetCallbackDeviceChanging(modifyDeviceSettings);
    DXUTSetCallbackMsgProc(msgProc);
    DXUTSetCallbackKeyboard(keyboardProc);
    DXUTSetCallbackFrameMove(onFrameMove);

    initApp();
    DXUTInit(true, true);
    DXUTSetCursorSettings(true, true);
    DXUTCreateWindow(L"Separable SSS Demo");
    DXUTCreateDevice(D3D_FEATURE_LEVEL_11_0, true, 1280, 720);
    DXUTMainLoop();

    return DXUTGetExitCode();
}
