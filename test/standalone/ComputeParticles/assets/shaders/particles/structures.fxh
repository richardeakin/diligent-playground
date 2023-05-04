
struct ParticleAttribs {
    float3 pos;
    float  padding0;
    float3 newPos;
    float  padding1;

    float3 vel;
    float  padding2;
    float3 newVel;
    float  padding3;

    float3  accel;
    float   padding4;
    float3  newAccel;
    float   padding5;

    float  size;
    float  temperature;
    int    numInteractions;
    float  padding6;
};

struct ParticleConstants {
    float4x4 viewProj; // TODO: should be removed? Or moved to the sprites-only buffer

    uint    numParticles;
    float   deltaTime;
    float   separation;
    float   padding0;

    float   scale; // TODO: make float3 so I can use (0.4, 1, 0.4) to shape the pyramid
    int3    gridSize;
    
    float2  speedMinMax;
    float   alignment;
    float   cohesion;

    float   separationDist;
    float   alignmentDist;
    float   cohesionDist;
    float   padding1;
};

// matches struct from solids/solid.fxh
struct SceneConstants {
    float4x4 ModelViewProj;
    float4x4 NormalTranform;
    float4   LightDirection;
};
