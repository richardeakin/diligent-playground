#include "shaders/particles/structures.fxh"

cbuffer VSConstants {
    float4x4 WorldViewProj;
    float4x4 NormalTranform;
    float4   LightDirection;
};

cbuffer PConstants {
    ParticleConstants PConstants;
};

StructuredBuffer<ParticleAttribs> Particles;

struct VSInput {
    float3 Pos : ATTRIB0;
    float3 Normal : ATTRIB1;
    float2 UV  : ATTRIB2;

    // TODO: are these set automatically by DX?
    //uint VertID : SV_VertexID;
    uint InstID : SV_InstanceID;
};

struct PSInput { 
    float4 Pos  : SV_POSITION; 
    float2 UV   : TEX_COORD;
    float  Temp : TEMPERATURE;
    float  NdotL : N_DOT_L;
};

void main( in VSInput VSIn, out PSInput PSIn )
{
    //float4 pos_uv[4];
    //pos_uv[0] = float4(-1.0,+1.0, 0.0,0.0);
    //pos_uv[1] = float4(-1.0,-1.0, 0.0,1.0);
    //pos_uv[2] = float4(+1.0,+1.0, 1.0,0.0);
    //pos_uv[3] = float4(+1.0,-1.0, 1.0,1.0);

    ParticleAttribs Attribs = Particles[VSIn.InstID];

    float3 pos = VSIn.Pos;
    pos = pos * Attribs.size * PConstants.scale + Attribs.pos;

    //PSIn.Pos = mul( float4( pos, 1.0 ), Constants.viewProj );
    //PSIn.uv = pos_uv[VSIn.VertID].zw;

    PSIn.Pos = mul( float4( pos, 1.0 ), WorldViewProj );
    PSIn.UV  = VSIn.UV;
    PSIn.Temp = Attribs.temperature;

    float3 N = mul( float4( VSIn.Normal, 0.0 ), NormalTranform ).xyz;
    PSIn.NdotL = saturate( dot( N, -LightDirection.xyz ) );
}
