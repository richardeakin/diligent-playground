
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
    
    int4 bin; // TODO: remove after debugging
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
    float   sdfAvoidStrength;

    float3  worldMin;
    float   sdfAvoidDistance;

    float3  worldMax;
    float   padding2;
};

struct BoidPSInput { 
    float4 Pos      : SV_POSITION; 
    float2 UV       : TEX_COORD;
    float  Temp     : TEMPERATURE;
    float3 Normal   : NORMAL;
    float  Movement : MOVEMENT;
    uint   InstID   : INSTANCE_ID;
    int4   Bin      : PARTICLE_BIN;
};


// matches struct from solids/solid.fxh
struct SceneConstants {
    float4x4 ModelViewProj;
    float4x4 NormalTranform;
    float4   LightDirection;
};
