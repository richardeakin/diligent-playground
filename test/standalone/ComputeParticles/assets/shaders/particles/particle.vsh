#include "shaders/particles/structures.fxh"

cbuffer Constants
{
    GlobalConstants g_Constants;
};

StructuredBuffer<ParticleAttribs> g_Particles;

struct VSInput
{
    uint VertID : SV_VertexID;
    uint InstID : SV_InstanceID;
};

struct PSInput 
{ 
    float4 Pos  : SV_POSITION; 
    float2 uv   : TEX_COORD;
    float  Temp : TEMPERATURE;
};

void main(in  VSInput VSIn,
          out PSInput PSIn)
{
    float4 pos_uv[4];
    pos_uv[0] = float4(-1.0,+1.0, 0.0,0.0);
    pos_uv[1] = float4(-1.0,-1.0, 0.0,1.0);
    pos_uv[2] = float4(+1.0,+1.0, 1.0,0.0);
    pos_uv[3] = float4(+1.0,-1.0, 1.0,1.0);

    ParticleAttribs Attribs = g_Particles[VSIn.InstID];

    // TODO: make sure I understand what scale is
    float3 pos = float3( pos_uv[VSIn.VertID].xy * g_Constants.f2Scale.xy, 0.0 );
    pos = pos * Attribs.fSize + Attribs.pos;
    PSIn.Pos = float4(pos, 1.0);
    PSIn.uv = pos_uv[VSIn.VertID].zw;
    PSIn.Temp = Attribs.fTemperature;
}
