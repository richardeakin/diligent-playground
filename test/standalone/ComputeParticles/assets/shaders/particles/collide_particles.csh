#include "shaders/particles/structures.fxh"
#include "shaders/particles/particles.fxh"

#define BINNING_ENABLED 1

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

// https://en.wikipedia.org/wiki/Elastic_collision
void CollideParticles(inout ParticleAttribs P0, in ParticleAttribs P1)
{
    // TODO: update for 3D
    float3 R01 = ( P1.pos - P0.pos ) / Constants.scale;
    float d01 = length( R01 );
    R01 /= d01;
    if( d01 < P0.size + P1.size ) {
#if UPDATE_SPEED
        // The math for speed update is only valid for two-particle collisions.
        if( P0.numCollisions == 1 && P1.numCollisions == 1 ) {
            float v0 = dot( P0.speed, R01 );
            float v1 = dot( P1.speed, R01 );

            float m0 = P0.size * P0.size;
            float m1 = P1.size * P1.size;

            float new_v0 = ((m0 - m1) * v0 + 2.0 * m1 * v1) / (m0 + m1);
            P0.newSpeed += (new_v0 - v0) * R01;
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

[numthreads(THREAD_GROUP_SIZE, 1, 1)]
void main( uint3 Gid  : SV_GroupID,
           uint3 GTid : SV_GroupThreadID )
{
    uint uiGlobalThreadIdx = Gid.x * uint(THREAD_GROUP_SIZE) + GTid.x;
    if( uiGlobalThreadIdx >= Constants.numParticles )
        return;

    int iParticleIdx = int(uiGlobalThreadIdx);
    ParticleAttribs Particle = Particles[iParticleIdx];
    
    const int3 gridSize = Constants.gridSize;
    const int4 gridLoc = GetGridLocation( Particle.pos, Constants.gridSize );

#if ! UPDATE_SPEED
    Particle.newPos       = Particle.pos;
    Particle.numCollisions = 0;
#else
    Particle.newSpeed     = Particle.speed;
    // Only update speed when there is single collision with another particle.
    if( Particle.numCollisions == 1 ) {
#endif

#if BINNING_ENABLED
        {
        int z = 0; // FIXME: traversing in z is causing a runtime crash. could mean a loop
        //for( int z = max( gridLoc.z - 1, 0 ); z <= min( gridLoc.z + 1, gridSize.z - 1 ); ++z ) {
            for( int y = max( gridLoc.y - 1, 0 ); y <= min( gridLoc.y + 1, gridSize.y - 1 ); ++y ) {
                for( int x = max( gridLoc.x - 1, 0 ); x <= min( gridLoc.x + 1, gridSize.x - 1 ); ++x ) {
                    int neighborIndex = Grid3DTo1D( int3( x, y, z ), gridSize );
                    int AnotherParticleIdx = ParticleListHead.Load( neighborIndex );
                    while( AnotherParticleIdx >= 0 ) {
                        if( iParticleIdx != AnotherParticleIdx ) {
                            ParticleAttribs AnotherParticle = Particles[AnotherParticleIdx];
                            CollideParticles( Particle, AnotherParticle );
                        }

                        AnotherParticleIdx = ParticleLists.Load( AnotherParticleIdx );
                    }
                }
            }
        }
#else
        // brute-force try to collide all particles to eachother
        for( int i = 0; i < Constants.numParticles; i++ ) {
            if( i == iParticleIdx ) {
                continue;
            }
            CollideParticles( Particle, Particles[i] );
        }
#endif

#if UPDATE_SPEED
    }
    else if( Particle.numCollisions > 1 ) {
        // If there are multiple collisions, reverse the particle move direction to avoid particle crowding.
        Particle.newSpeed = -Particle.speed;
    }
#else
    ClampParticlePosition( Particle.newPos, Particle.speed, Particle.size * Constants.scale );
#endif

    Particles[iParticleIdx] = Particle;
}
