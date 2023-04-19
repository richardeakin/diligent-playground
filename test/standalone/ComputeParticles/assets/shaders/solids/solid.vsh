
#include "shaders/solids/solid.fxh"

cbuffer SConstants {
    SceneConstants SConstants;
};

struct VSInput {
    float3 Pos : ATTRIB0;
    float3 Normal : ATTRIB1;
    float2 UV  : ATTRIB2;
};

void main( in VSInput VSIn, out PSInput PSIn ) 
{
    PSIn.Pos = mul( float4( VSIn.Pos, 1.0 ), SConstants.ModelViewProj );
    PSIn.UV  = VSIn.UV;

    PSIn.Normal = mul( float4( VSIn.Normal, 0.0 ), SConstants.NormalTranform ).xyz;
    //PSIn.NdotL = saturate( dot( N, - LightDirection.xyz ) );
}
