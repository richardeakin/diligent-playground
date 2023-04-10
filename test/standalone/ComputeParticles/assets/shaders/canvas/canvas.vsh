cbuffer Constants {
    float2 cCenter;
    float2 cSize;
};

struct VSInput {
    uint VertexID : SV_VertexID;
};

struct PSInput {
    float4 Pos    : SV_POSITION;
    float2 UV     : TEX_COORD;
};

void main( in VSInput VSIn, out PSInput PSIn )
{
    float2 Pos[4];
    Pos[0] = float2(-1.0, -1.0);
    Pos[1] = float2(-1.0, +1.0);
    Pos[2] = float2(+1.0, -1.0);
    Pos[3] = float2(+1.0, +1.0);

    // float2 Center = float2(-0.6, -0.6);
    // float2 Size   = float2(0.35, 0.35);

    PSIn.Pos = float4( cCenter + cSize * Pos[VSIn.VertexID], 0.0, 1.0 );
    PSIn.UV  = Pos[VSIn.VertexID].xy * F3NDC_XYZ_TO_UVD_SCALE.xy + float2(0.5, 0.5);
}
