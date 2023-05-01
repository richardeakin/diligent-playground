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

#ifndef UPDATE_SPEED
#   define UPDATE_SPEED 0
#endif

RWStructuredBuffer<ParticleAttribs> Particles;
Buffer<int>                         ParticleListHead;
Buffer<int>                         ParticleLists;

#if 0
// https://en.wikipedia.org/wiki/Elastic_collision
void CollideParticles(inout ParticleAttribs P0, in ParticleAttribs P1)
{
    float3 R01 = ( P1.pos - P0.pos ) / Constants.scale;
    float d01 = length( R01 );
    R01 /= d01;
    if( d01 < P0.size + P1.size ) {
#if UPDATE_SPEED
        // The math for speed update is only valid for two-particle collisions.
        if( P0.numCollisions == 1 && P1.numCollisions == 1 ) {
            float v0 = dot( P0.vel, R01 );
            float v1 = dot( P1.vel, R01 );

            float m0 = P0.size * P0.size;
            float m1 = P1.size * P1.size;

            float new_v0 = ((m0 - m1) * v0 + 2.0 * m1 * v1) / (m0 + m1);
            P0.newVel += (new_v0 - v0) * R01;
        }
#else
        {
            // Move the particle away
            P0.newPos += -R01 * (P0.size + P1.size - d01) * Constants.scale * 0.51;

            // Set our fake temperature to 1 to indicate collision
            P0.temperature = 1.0;

            // Count the number of collisions
            P0.numCollisions += 1;
        }
#endif
    }
}
#endif

void InteractParticles( inout ParticleAttribs p0, in ParticleAttribs p1 )
{
    float3 r10 = ( p1.pos - p0.pos );
    float dist = length( r10 ); // TODO (optimiziation): use dist squared
    if( dist < Constants.zoneRadius ) {
#if UPDATE_SPEED
        // TODO: finish
#else
        // TODO: finish
#endif
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

#if ! UPDATE_SPEED
    particle.newPos       = particle.pos;
    particle.numCollisions = 0;
#else
    particle.newVel     = particle.vel;
    // Only update speed when there is single collision with another particle.
    if( particle.numCollisions == 1 ) {
#endif

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
                    CollideParticles( particle, anotherParticle );
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
                            CollideParticles( particle, anotherParticle );
                        }

                        anotherParticleId = ParticleLists.Load( anotherParticleId );
                    }
                }
            }
        }
#endif

#if UPDATE_SPEED
    }
    else if( particle.numCollisions > 1 ) {
        // If there are multiple collisions, reverse the particle move direction to avoid particle crowding.
        particle.newVel = -particle.vel;
    }
#else
    ClampParticlePosition( particle.newPos, particle.vel, particle.size * Constants.scale );
#endif

    Particles[particleId] = particle;
}
