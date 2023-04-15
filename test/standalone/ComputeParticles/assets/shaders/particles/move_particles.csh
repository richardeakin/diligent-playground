#include "shaders/particles/structures.fxh"
#include "shaders/particles/particles.fxh"

cbuffer Constants
{
    GlobalConstants g_Constants;
};

#ifndef THREAD_GROUP_SIZE
#   define THREAD_GROUP_SIZE 64
#endif

RWStructuredBuffer<ParticleAttribs> g_Particles;
RWBuffer<int /*format=r32i*/>       g_ParticleListHead;
RWBuffer<int /*format=r32i*/>       g_ParticleLists;

[numthreads(THREAD_GROUP_SIZE, 1, 1)]
void main(uint3 Gid  : SV_GroupID,
          uint3 GTid : SV_GroupThreadID)
{
    uint uiGlobalThreadIdx = Gid.x * uint(THREAD_GROUP_SIZE) + GTid.x;
    if (uiGlobalThreadIdx >= g_Constants.uiNumParticles)
        return;

    int iParticleIdx = int(uiGlobalThreadIdx);

    ParticleAttribs Particle = g_Particles[iParticleIdx];
    Particle.pos   = Particle.newPos;
    Particle.speed = Particle.newSpeed;
    //Particle.pos  += Particle.speed * g_Constants.scale * g_Constants.fDeltaTime; // TODO: make scale float3
    Particle.pos  += Particle.speed * float3( g_Constants.f2Scale, 0 ) * g_Constants.fDeltaTime;
    Particle.fTemperature -= Particle.fTemperature * min(g_Constants.fDeltaTime * 2.0, 1.0);

    ClampParticlePosition( Particle.pos, Particle.speed, Particle.fSize, g_Constants.f2Scale );
    g_Particles[iParticleIdx] = Particle;

    // Bin particles. TODO: make grid 3D
    int GridIdx = GetGridLocation(Particle.pos.xy, g_Constants.i2ParticleGridSize).z;
    int OriginalListIdx;
    InterlockedExchange(g_ParticleListHead[GridIdx], iParticleIdx, OriginalListIdx);
    g_ParticleLists[iParticleIdx] = OriginalListIdx;
}
