#include "shaders/particles/structures.fxh"
#include "shaders/particles/particles.fxh"

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

    int particleIdx = int(globalThreadIdx);

    ParticleAttribs Particle = Particles[particleIdx];
    Particle.pos   = Particle.newPos;
    Particle.vel = Particle.newVel;
    Particle.pos  += Particle.vel * Constants.scale * Constants.deltaTime;
    Particle.temperature -= Particle.temperature * min( Constants.deltaTime * 2.0, 1.0 );

    ClampParticlePosition( Particle.pos, Particle.vel, Particle.size * Constants.scale );
    Particles[particleIdx] = Particle;

    int GridIdx = GetGridLocation( Particle.pos, Constants.gridSize ).w;
    int OriginalListIdx;
    InterlockedExchange( ParticleListHead[GridIdx], particleIdx, OriginalListIdx );
    ParticleLists[particleIdx] = OriginalListIdx;
}
