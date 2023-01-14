#include "skin.hlsli"

void main(
	in Vertex i_vtx,
	in float3 i_vecCamera
	: CAMERA,
	  in float4 i_uvzwShadow
	: UVZW_SHADOW,
	  out float3 o_rgbLit
	: SV_Target)
{
	SkinMegashader(i_vtx, i_vecCamera, i_uvzwShadow, o_rgbLit, false, true);
}