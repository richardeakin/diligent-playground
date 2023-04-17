
cbuffer VSConstants {
    float4x4 WorldViewProj;
    float4x4 NormalTranform;
    float4   LightDirection;
};

struct VSInput {
    float3 Pos : ATTRIB0;
    float3 Normal : ATTRIB1;
    float2 UV  : ATTRIB2;
};

struct PSInput { 
    float4 Pos : SV_POSITION;
    float2 UV  : TEX_COORD; 
    float  NdotL : N_DOT_L;
};

void main( in VSInput VSIn, out PSInput PSIn ) 
{
    PSIn.Pos = mul( float4( VSIn.Pos, 1.0 ), WorldViewProj );
    PSIn.UV  = VSIn.UV;

    float3 N = mul( float4( VSIn.Normal, 0.0 ), NormalTranform ).xyz;
    PSIn.NdotL = saturate( dot( N, - LightDirection.xyz ) );
}
