
// TODO: try without padding between the positions
struct ParticleAttribs {
    float3 pos;
    float  padding0;
    float3 newPos;
    float  padding1;

    float3 speed;
    float  padding2;
    float3 newSpeed;
    float  padding3;

    float  fSize;
    float  fTemperature;
    int    iNumCollisions;
    float  fPadding4;
};

struct GlobalConstants {
    float4x4 cViewProj;

    uint   uiNumParticles;
    float  fDeltaTime;
    float  fDummy0;
    float  fDummy1;

    float2 f2Scale;
    int2   i2ParticleGridSize;
};
