struct SceneConstants {
    float4x4 ModelViewProj;
    float4x4 NormalTranform;
    float4   LightDirection;
};

struct PSInput { 
    float4 Pos   : SV_POSITION; 
    float2 UV  : TEX_COORD; 
    float3  Normal : NORMAL;
};
