
// TODO: try without padding between the positions
struct ParticleAttribs {
    float3 pos;
    float  padding0;
    float3 newPos;
    float  padding1;

    float3 vel;
    float  padding2;
    float3 newVel;
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
    float  zoneRadius;
    float  padding1;

    float  scale; // TODO: make float3 so I can use (0.4, 1, 0.4) to shape the pyramid
    int3   gridSize;
};

// matches struct from solids/solid.fxh
struct SceneConstants {
    float4x4 ModelViewProj;
    float4x4 NormalTranform;
    float4   LightDirection;
};
