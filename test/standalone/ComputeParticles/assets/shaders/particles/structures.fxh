
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

    float  size;
    float  temperature;
    int    numCollisions;
    float  padding4;
};

struct ParticleConstants {
    float4x4 viewProj; // TODO: should be removed? Or moved to the sprites-only buffer

    uint   numParticles;
    float  deltaTime;
    float  padding0;
    float  padding1;

    float  scale;
    int3   gridSize;
};

// matches struct from solids/solid.fxh
struct SceneConstants {
    float4x4 ModelViewProj;
    float4x4 NormalTranform;
    float4   LightDirection;
};
