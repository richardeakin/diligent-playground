#include "shaders/particles/structures.fxh"
#include "shaders/particles/particles.fxh"

// 0: disabled, 1: only consider this particle's bin, 2: also consider neighboring bins
#define BINNING_MODE 0

cbuffer Constants {
    ParticleConstants Constants;
};

#ifndef THREAD_GROUP_SIZE
#   define THREAD_GROUP_SIZE 64
#endif

RWStructuredBuffer<ParticleAttribs> Particles;
Buffer<int>                         ParticleListHead;
Buffer<int>                         ParticleLists;

void InteractParticles( inout ParticleAttribs p0, in ParticleAttribs p1 )
{
    float3 r10 = ( p1.pos - p0.pos );
    float dist = length( r10 ); // TODO (optimiziation): use dist squared
    float maxDist = Constants.cohesionDist;
    if( dist < maxDist ) {
        float3 d10 = normalize( r10 );
        if( dist < Constants.separationDist ) {
            // add force that flies p0 away from p1
            float F = ( Constants.separationDist / dist - 1.0f ) * Constants.separation;
            p0.accel -= d10 * F;
        }
        else if( dist < Constants.alignmentDist ) {
            // add force that flies p0 in same direction as p1
            float3 dir = normalize( p1.vel );
            float F = ( Constants.alignmentDist / dist - 1.0f ) * Constants.alignment;
            p0.accel += dir * F;
        }
        else if( dist < Constants.cohesionDist ) {
            // add force that flies p0 towards p1
            float F = ( Constants.cohesionDist / dist - 1.0f ) * Constants.cohesion;
            p0.accel += d10 * F;
        }
    }
}

[numthreads(THREAD_GROUP_SIZE, 1, 1)]
void main( uint3 Gid  : SV_GroupID,
           uint3 GTid : SV_GroupThreadID )
{
    uint globalThreadId = Gid.x * uint(THREAD_GROUP_SIZE) + GTid.x;
    if( globalThreadId >= Constants.numParticles )
        return;

    int particleId = int(globalThreadId);
    ParticleAttribs particle = Particles[particleId];
    
    const int3 gridSize = Constants.gridSize;
    const int4 gridLoc = GetGridLocation( particle.pos, Constants.gridSize );

    particle.accel = 0.0; // TODO: should we be adding to newAccel?

    // TODO: understand why I need to set newPos / newVel == old pos / vel here for them to move
    // - might be wrong once move_particles is using integration
    particle.newPos         = particle.pos;
    particle.newVel         = particle.vel;
    particle.numCollisions  = 0;
    
#if BINNING_MODE == 0
    // brute-force try to collide all particles to eachother
    for( int i = 0; i < Constants.numParticles; i++ ) {
        if( i == particleId ) {
            continue;
        }
        //CollideParticles( particle, Particles[i] );
        InteractParticles( particle, Particles[i] );
    }
#elif BINNING_MODE == 1
    // only considering particles within the same bin
    {
        int anotherParticleId = ParticleListHead.Load( particleId );
        while( anotherParticleId >= 0 ) {
            if( particleId != anotherParticleId ) {
                ParticleAttribs anotherParticle = Particles[anotherParticleId];
                InteractParticles( particle, anotherParticle );
            }

            anotherParticleId = ParticleLists.Load( anotherParticleId );
        }
    }
#else
    //{
    //int z = 0; // FIXME: traversing in z is causing a runtime crash. could mean a loop
    for( int z = max( gridLoc.z - 1, 0 ); z <= min( gridLoc.z + 1, gridSize.z - 1 ); ++z ) {
        for( int y = max( gridLoc.y - 1, 0 ); y <= min( gridLoc.y + 1, gridSize.y - 1 ); ++y ) {
            for( int x = max( gridLoc.x - 1, 0 ); x <= min( gridLoc.x + 1, gridSize.x - 1 ); ++x ) {
                int neighborIndex = Grid3DTo1D( int3( x, y, z ), gridSize );
                int anotherParticleId = ParticleListHead.Load( neighborIndex );
                while( anotherParticleId >= 0 ) {
                    if( particleId != anotherParticleId ) {
                        ParticleAttribs anotherParticle = Particles[anotherParticleId];
                        InteractParticles( particle, anotherParticle );
                    }

                    anotherParticleId = ParticleLists.Load( anotherParticleId );
                }
            }
        }
    }
#endif

    particle.newVel += particle.accel * Constants.deltaTime;

    // TODO: needed? think it is done in the move pass
    //ClampParticlePosition( particle.newPos, particle.vel, particle.size * Constants.scale );

    Particles[particleId] = particle;
}
