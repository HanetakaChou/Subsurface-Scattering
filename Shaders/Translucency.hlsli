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

 //-----------------------------------------------------------------------------
 // Porting Functions

#define SSSSSaturate(a) saturate(a)

//-----------------------------------------------------------------------------
// Separable SSS Transmittance Function

float3 SSSSTransmittance(
	float translucency,
	float sssWidth,
	// Normal in world space.
	float3 N,
	// Light vector: lightWorldPosition - worldPosition.
	float3 L,
	// Thickness in world units.
	float dInUnits)
{
	float metersPerUnit = 3.0 * 0.001 * 0.5 / sssWidth;

	// The shader code is merely to transform the thickness from world units to mm.
	// Actually, the default of "translucency" is 0.83 and the result of "8.25 * (1.0 - translucency) * 2.0  * 1000.0 / 3.0" is 0.935 which is really close to 1.
	float scale = 8.25 * (1.0 - translucency) * 2.0 / 3.0;
	scale *= (1000.0 * metersPerUnit);

	float d = scale * dInUnits;

	// The transmittance coefficient T(d) is precalculated based on diffusion profile which is approximated by the Gaussians.
	// It can be precomputed into a texture, for maximum performance
	float dd = -d * d;
	float3 T = float3(0.233, 0.455, 0.649) * exp(dd / 0.0064) +
		float3(0.1, 0.336, 0.344) * exp(dd / 0.0484) +
		float3(0.118, 0.198, 0.0) * exp(dd / 0.187) +
		float3(0.113, 0.007, 0.007) * exp(dd / 0.567) +
		float3(0.358, 0.004, 0.0) * exp(dd / 1.99) +
		float3(0.078, 0.0, 0.0) * exp(dd / 7.41);

	// Using the transmittance coefficient T(d), we finally approximate the transmitted lighting from the back of the object
	float NoL = SSSSSaturate(0.3 + dot(-N, L));

	return T * NoL;
}