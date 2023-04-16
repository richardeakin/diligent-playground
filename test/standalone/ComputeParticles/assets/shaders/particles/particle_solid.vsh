#include "shaders/particles/structures.fxh"

cbuffer Constants {
    GlobalConstants Constants;
};

StructuredBuffer<ParticleAttribs> Particles;

struct VSInput {
    uint VertID : SV_VertexID;
    uint InstID : SV_InstanceID;
};

struct PSInput { 
    float4 Pos  : SV_POSITION; 
    float2 uv   : TEX_COORD;
    float  Temp : TEMPERATURE;
};

void main( in VSInput VSIn, out PSInput PSIn )
{
    float4 pos_uv[4];
    pos_uv[0] = float4(-1.0,+1.0, 0.0,0.0);
    pos_uv[1] = float4(-1.0,-1.0, 0.0,1.0);
    pos_uv[2] = float4(+1.0,+1.0, 1.0,0.0);
    pos_uv[3] = float4(+1.0,-1.0, 1.0,1.0);

    ParticleAttribs Attribs = Particles[VSIn.InstID];

    float3 pos = float3( pos_uv[VSIn.VertID].xy * Constants.scale, 0.0 ); // TODO: use pos.z
    pos = pos * Attribs.size + Attribs.pos;
    PSIn.Pos = mul( float4( pos, 1.0 ), Constants.viewProj );
    PSIn.uv = pos_uv[VSIn.VertID].zw;
    PSIn.Temp = Attribs.temperature;
}
