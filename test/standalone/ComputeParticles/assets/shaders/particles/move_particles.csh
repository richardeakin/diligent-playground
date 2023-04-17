#include "shaders/particles/structures.fxh"
#include "shaders/particles/particles.fxh"
#line 4

cbuffer Constants {
    ParticleConstants Constants;
};

#ifndef THREAD_GROUP_SIZE
#   define THREAD_GROUP_SIZE 64
#endif

RWStructuredBuffer<ParticleAttribs> Particles;
RWBuffer<int /*format=r32i*/>       ParticleListHead;
RWBuffer<int /*format=r32i*/>       ParticleLists;

[numthreads(THREAD_GROUP_SIZE, 1, 1)]
void main( uint3 Gid  : SV_GroupID,
           uint3 GTid : SV_GroupThreadID)
{
    uint globalThreadIdx = Gid.x * uint(THREAD_GROUP_SIZE) + GTid.x;
    if( globalThreadIdx >= Constants.numParticles ) {
        return;
    }

    int iParticleIdx = int(globalThreadIdx);

    ParticleAttribs Particle = Particles[iParticleIdx];
    Particle.pos   = Particle.newPos;
    Particle.speed = Particle.newSpeed;
    Particle.pos  += Particle.speed * Constants.scale * Constants.deltaTime;
    Particle.temperature -= Particle.temperature * min( Constants.deltaTime * 2.0, 1.0 );

    ClampParticlePosition( Particle.pos, Particle.speed, Particle.size * Constants.scale );
    Particles[iParticleIdx] = Particle;

    // Bin particles. TODO: make grid 3D. May have to use .w for the id then
    int GridIdx = GetGridLocation( Particle.pos.xy, Constants.gridSize ).z;
    int OriginalListIdx;
    InterlockedExchange( ParticleListHead[GridIdx], iParticleIdx, OriginalListIdx );
    ParticleLists[iParticleIdx] = OriginalListIdx;
}
