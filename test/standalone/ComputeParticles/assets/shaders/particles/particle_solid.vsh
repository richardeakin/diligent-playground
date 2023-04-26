#include "shaders/particles/structures.fxh"
#include "shaders/Quaternion.hlsl"

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

    // scale pyramid to look more like an arrow
    // TODO: use PConstants.scale instead, make it a float3
    float3 scale = float3( 0.4, 1.0, 0.4 ) * PConstants.scale;

    //pos = pos * Attribs.size * PConstants.scale + Attribs.pos;
    pos = pos * Attribs.size * scale;


    // TODO NEXT: try my cpp method for the rotation
    // rotate in the direction of current speed
    float3 lookAtDir = normalize( Attribs.speed );
    float4 lookAtQuat = q_look_at( lookAtDir, float3( 0, 1, 0 ) );
    float4x4 lookAtMat = quaternion_to_matrix( lookAtQuat );
    float4 posRotated = mul( float4( pos, 1.0 ), lookAtMat );


    //posRotated += float4( Attribs.pos, 0 );
    //PSIn.Pos = mul( posRotated, SConstants.ModelViewProj );

    PSIn.Pos = mul( float4( pos + Attribs.pos, 1.0 ), SConstants.ModelViewProj );

    PSIn.UV  = VSIn.UV;
    PSIn.Temp = Attribs.temperature;

    PSIn.Normal = mul( float4( VSIn.Normal, 0.0 ), SConstants.NormalTranform ).xyz;
}
