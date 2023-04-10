cbuffer Constants {
    float2 cSize; // TODO: use to set positions. But first check out rendertarget is drawn fullscreen without
    float2 cPadding;
};

struct VSInput {
    uint VertexID : SV_VertexID;
};

struct PSInput {
    float4 Pos    : SV_POSITION;
    float2 UV     : TEX_COORD;
};

void main( in  VSInput VSIn, out PSInput PSIn )
{
    float4 Pos[4];
    // Pos[0] = float4(-1.0, -1.0, 0.0, 1.0);
    Pos[0] = float4(-cSize.x, -cSize.y, 0.0, 1.0); // TODO: this is now double the size
    Pos[1] = float4(-cSize.x, +cSize.y, 0.0, 1.0);
    Pos[2] = float4(+cSize.x, -cSize.y, 0.0, 1.0);
    Pos[3] = float4(+cSize.x, +cSize.y, 0.0, 1.0);

    float2 UV[4];
    UV[0] = float2(+0.0, +1.0);
    UV[1] = float2(+0.0, +0.0);
    UV[2] = float2(+1.0, +1.0);
    UV[3] = float2(+1.0, +0.0);

    PSIn.Pos = Pos[VSIn.VertexID];
    PSIn.UV = UV[VSIn.VertexID];
}
