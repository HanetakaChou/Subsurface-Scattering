Texture2D specularsTex : register(t0);

SamplerState PointSampler : register(s0)
{
	Filter = MIN_MAG_MIP_POINT;
	AddressU = Clamp;
	AddressV = Clamp;
};

void PassVS(float4 position : POSITION,
	out float4 svposition : SV_POSITION,
	inout float2 texcoord : TEXCOORD0) {
	svposition = position;
}

float4 AddSpecularPS(float4 position : SV_POSITION,
	float2 texcoord : TEXCOORD) : SV_TARGET{
		return specularsTex.Sample(PointSampler, texcoord);
}
