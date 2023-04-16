
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

struct GlobalConstants {
    float4x4 viewProj;

    uint   numParticles;
    float  deltaTime;
    float  padding0;
    float  padding1;

    float  scale;
    int2   gridSize; // TODO: make 3D
};
