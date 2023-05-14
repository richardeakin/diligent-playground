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

    float speed = length( particle.newVel );
    float3 dir = particle.newVel / speed; // TODO: do clamp speed line first and set min speed to a positive value to avoid divide by zero
    speed = clamp( speed, Constants.speedMinMax.x, Constants.speedMinMax.y );
    float3 vel = dir * speed;
    //vel = particle.newVel;

    particle.vel = vel;
    particle.pos   = particle.newPos + vel * Constants.deltaTime;
    particle.temperature = particle.numInteractions / 10.0;


    ClampParticlePosition( particle.pos, particle.vel, particle.size * Constants.scale, Constants.worldMin, Constants.worldMax );
    Particles[particleId] = particle;

    int gridId = GetGridLocation( particle.pos, Constants.gridSize ).w;

    // swap list head with this, move previous down the list one
    int originalListId;
    InterlockedExchange( ParticleListHead[gridId], particleId, originalListId );
    ParticleLists[particleId] = originalListId;
}
