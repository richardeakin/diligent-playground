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
    uint globalThreadId = Gid.x * uint(THREAD_GROUP_SIZE) + GTid.x;
    if( globalThreadId >= Constants.numParticles ) {
        return;
    }

    int particleId = int(globalThreadId);

    ParticleAttribs particle = Particles[particleId];
    particle.pos   = particle.newPos;
    particle.vel = particle.newVel;
    particle.pos  += particle.vel * Constants.deltaTime;
    particle.temperature -= particle.temperature * min( Constants.deltaTime * 2.0, 1.0 );

    ClampParticlePosition( particle.pos, particle.vel, particle.size * Constants.scale );
    Particles[particleId] = particle;

    int gridId = GetGridLocation( particle.pos, Constants.gridSize ).w;

    // swap list head with this, move previous down the list one
    int originalListId;
    InterlockedExchange( ParticleListHead[gridId], particleId, originalListId );
    ParticleLists[particleId] = originalListId;
}
