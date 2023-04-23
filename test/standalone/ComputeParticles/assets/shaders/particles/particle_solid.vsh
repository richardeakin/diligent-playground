#include "shaders/particles/structures.fxh"

cbuffer SConstants {
    SceneConstants SConstants;
};

cbuffer PConstants {
    ParticleConstants PConstants;
};

StructuredBuffer<ParticleAttribs> Particles;

struct VSInput {
    float3 Pos : ATTRIB0;
    float3 Normal : ATTRIB1;
    float2 UV  : ATTRIB2;

    uint InstID : SV_InstanceID;
};

struct PSInput { 
    float4 Pos  : SV_POSITION; 
    float2 UV   : TEX_COORD;
    float  Temp : TEMPERATURE;
    float3 Normal : NORMAL;
};

void main( in VSInput VSIn, out PSInput PSIn )
{
    ParticleAttribs Attribs = Particles[VSIn.InstID];

    float3 pos = VSIn.Pos;
    pos = pos * Attribs.size * PConstants.scale + Attribs.pos;

    PSIn.Pos = mul( float4( pos, 1.0 ), SConstants.ModelViewProj );
    PSIn.UV  = VSIn.UV;
    PSIn.Temp = Attribs.temperature;

    PSIn.Normal = mul( float4( VSIn.Normal, 0.0 ), SConstants.NormalTranform ).xyz;
}
