
struct ParticleAttribs {
    float3 pos;
    float  padding0;
    float3 newPos;
    float  distToSDF;

    float3 vel;
    float  padding2;
    float3 newVel;
    float  padding3;

    float3  accel;
    float   sdfRayLength;
    float3  newAccel;
    int     sdfIterations;

    float  size;
    float  temperature;
    int    numInteractions;
    int    nearestSDFObject;
    
    float3 sdfClosestNormal;
    float  sdfRepelStrength;
};

struct ParticleConstants {
    float4x4 viewProj; // TODO: should be removed? Or moved to the sprites-only buffer

    int     numParticles;
    float   time;
    float   deltaTime;
    float   separation;

    int3    gridSize;
    float   scale;
    
    float2  speedMinMax;
    float   alignment;
    float   cohesion;

    float   separationDist;
    float   alignmentDist;
    float   cohesionDist;
    float   padding0;

    float3  worldMin;
    float   padding1;

    float3  worldMax;
    float   padding2;
};

// matches struct from solids/solid.fxh
struct SceneConstants {
    float4x4 ModelViewProj;
    float4x4 NormalTranform;
    float4   LightDirection;
};
