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

float4 GetRotationFromAxisAngle( in float3 axis, in float angle )
{
    float4 result = float4( 0, 0, 0, 1 );
    const float norm = length( axis );
    if( norm != 0 ) {
        float sina2 = sin( 0.5 * angle );
        result[0]   = sina2 * axis[0] / norm;
        result[1]   = sina2 * axis[1] / norm;
        result[2]   = sina2 * axis[2] / norm;
        result[3]   = cos( 0.5 * angle );
    }
    return result;
}

// returns a quaternion that rotates vector a to vector b
float4 GetRotationQuat( in float3 a, in float3 b, in float3 up )
{   
    float dotAB = dot( a, b );
    // test for dot = -1
    if( abs( dotAB - (-1.0) ) < 0.000001 ) {
        // vector a and b point exactly in the opposite direction, 
        // so it is a 180 degrees turn around the up-axis
        return float4( up.x, up.y, up.z, PI );
    }
    // test for dot = 1
    if( abs( dotAB - (1.0) ) < 0.000001 ) {
        // vector a and b point exactly in the same direction
        // so we return the identity quaternion
        return float4( 0.0, 0.0, 0.0, 1.0 );
    }

    float rotAngle = acos( dotAB );
    float3 rotAxis = cross( a, b );
    rotAxis = normalize( rotAxis );

    return GetRotationFromAxisAngle( rotAxis, rotAngle );
    //return rotate_angle_axis( rotAngle, rotAxis );
}

void main( in VSInput VSIn, out PSInput PSIn )
{
    ParticleAttribs Attribs = Particles[VSIn.InstID];

    float3 pos = VSIn.Pos;

    // scale pyramid to look more like an arrow
    // TODO: make PConstants.scale a float3 so I can set it from UI
    float3 scale = float3( 0.4, 1.0, 0.4 ) * Attribs.size * PConstants.scale;

    //pos = pos * Attribs.size * PConstants.scale + Attribs.pos;
    pos *= scale;

    // rotate in the direction of current speed
    float3 lookAtDir = normalize( Attribs.speed );
    float4 lookAtQuat = GetRotationQuat( float3( 0, 1, 0 ), lookAtDir, float3( 0, 1, 0 ) );
    //float4 lookAtQuat = q_look_at( lookAtDir, float3( 0, 1, 0 ) );
    float4x4 lookAtMat = quaternion_to_matrix( lookAtQuat );
    //float4 posRotated = mul( float4( pos, 1.0 ), lookAtMat );
    float4 posRotated = mul( lookAtMat, float4( pos, 1.0 ) ); // TODO: figure out why post multiplying the pos fixes directions

    posRotated += float4( Attribs.pos, 0 );
    PSIn.Pos = mul( posRotated, SConstants.ModelViewProj );

    //PSIn.Pos = mul( float4( pos + Attribs.pos, 1.0 ), SConstants.ModelViewProj );

    PSIn.UV  = VSIn.UV;
    PSIn.Temp = Attribs.temperature;

    // TODO: normal also needs to be rotated
    PSIn.Normal = mul( float4( VSIn.Normal, 0.0 ), SConstants.NormalTranform ).xyz;
}
