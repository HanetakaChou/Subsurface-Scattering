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

#ifndef _SEPARABLE_SSS_H_
#define _SEPARABLE_SSS_H_ 1

#include <sdkddkver.h>
#define NOMINMAX 1
#define WIN32_LEAN_AND_MEAN 1
#include <DXUT.h>
#include <DirectXMath.h>
#include "RenderTarget.h"
#include <string>

class SeparableSSS
{
public:
    /**
     * width, height: should match the backbuffer's size.
     *
     * fovy: field of view angle used for rendering the scene, in degrees.
     *
     * sssWidth: specifies the global level of subsurface scattering, or in
     *     other words, the width of the filter.
     *
     * nSamples: number of samples of the kernel convolution.
     *
     * followShape: irradiance diffusion should occur on the surface of the
     *     object, not in a screen oriented plane. Setting 'followShape' to
     *     'true' will ensure that diffusion is more accurately calculated,
     *     at the expense of more memory accesses. This is usually only
     *     noticeable under specific lighting configurations.
     *
     * separateStrengthSource: this parameter enables to use a specific
     *     buffer for fetching the SSS strength, instead of using the alpha
     *     channel of the color buffer.
     *
     * stencilInitialized: if an stencil is initialized for selectively
     *     processing the frame buffer, set this to 'true'. For example,
     *     this cannot be easily done, when using multisampling. See
     *     @STENCIL below for more info.
     *
     * format: format of the temporal framebuffer. Should be left as is,
     *     unless an HDR format is desired.
     */
    SeparableSSS(ID3D11Device *device,
                 int width, int height,
                 float sssWidth,
                 int nSamples = 17,
                 DXGI_FORMAT format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB);
    ~SeparableSSS();

    /**
     * IMPORTANT NOTICE: all render targets below must not be multisampled.
     *
     * mainRTV and mainSRV: render target and shader resources of the
     *     final rendered image. The filter works on place, so they should
     *     be associated to the same resource. The SSS intensity should be
     *     stored in the alpha channel.
     *
     * depthSRV: shader resource of the the linear depth buffer of the
     *     scene, resolved in case of using multisampling. The resolve
     *     should be a simple average to avoid artifacts in the silhouette
     *     of objects.
     *
     * @STENCIL
     * depthDSV: two possibilities over here:
     *     a) if 'stencilInitialized' was set to 'true', then only the
     *        pixels marked in the stencil buffer with the value 'id' will
     *        be processed in both the horizontal and vertical passes.
     *     b) if 'stencilInitialized' was set to 'false' (for example when
     *        using multisampling), then it will be initialized on the fly
     *        for optimizing the second vertical pass.
     *
     * stregthSRV: if 'separateStrengthSource' was set to 'true' when
     *     creating this object, the SSS stregth will be fetched from the
     *     alpha channel of this texture instead of the alpha channel of
     *     the color buffer.
     *
     * id: stencil value used to render the objects we must apply
     *     subsurface scattering on. Should be left as is in case
     *     'stencilInitialized' was set to false when creating the object.
     *
     * Note that 'depthSRV' and 'depthDSV' cannot be the views of the same
     * depth-stencil buffer.
     */
    void go(ID3D11DeviceContext *context,
            ID3D11RenderTargetView *mainRTV,
            ID3D11ShaderResourceView *mainSRV,
            ID3D11ShaderResourceView *depthSRV,
            ID3D11DepthStencilView *depthDSV,
            ID3D11ShaderResourceView *stregthSRV = NULL);

    /**
     * This parameter specifies the global level of subsurface scattering,
     * or in other words, the width of the filter.
     */
    void setWidth(float width)
    {
        this->sssWidth = width;
    }

    /**
     * @STRENGTH
     *
     * This parameter specifies the how much of the diffuse light gets into
     * the skin, and thus gets modified by the SSS mechanism.
     *
     * It can be seen as a per-channel mix factor between the original
     * image, and the SSS-filtered image.
     */
    void setStrength(DirectX::XMFLOAT3 var_strength)
    {
        this->strength = var_strength;
        calculateKernel();
    }

    /**
     * This parameter defines the per-channel falloff of the gradients
     * produced by the subsurface scattering events.
     *
     * It can be used to fine tune the color of the gradients.
     */
    void setFalloff(DirectX::XMFLOAT3 var_falloff)
    {
        this->falloff = var_falloff;
        calculateKernel();
    }

    void setNSamples(int var_nSamples)
    {
        this->nSamples = var_nSamples;
        calculateKernel();
    }

    /**
     * This one is just for convenience, it returns the shader code of
     * current kernel.
     */
    std::string getKernelCode() const;

private:
    DirectX::XMFLOAT3 gaussian(float variance, float r);
    DirectX::XMFLOAT3 profile(float r);
    void calculateKernel();

    float sssWidth;
    int nSamples;
    bool stencilInitialized;
    DirectX::XMFLOAT3 strength;
    DirectX::XMFLOAT3 falloff;

    ID3D11VertexShader *SSSSBlurVS;
    ID3D11PixelShader *SSSSBlurXPS;
    ID3D11PixelShader *SSSSBlurYPS;
    ID3D11Buffer *CbufUpdatedPerFrame;
    ID3D11DepthStencilState *BlurStencil;
    ID3D11BlendState *NoBlending;
    ID3D11SamplerState *PointSampler;
    ID3D11SamplerState *LinearSampler;
    RenderTarget *tmpRT;
    Quad *quad;

    std::vector<DirectX::XMFLOAT4> kernel;
};

#endif
