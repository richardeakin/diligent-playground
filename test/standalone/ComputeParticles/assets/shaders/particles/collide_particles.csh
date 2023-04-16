#include "shaders/particles/structures.fxh"
#include "shaders/particles/particles.fxh"
#line 4

cbuffer Constants {
    GlobalConstants Constants;
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
    float2 R01 = ( P1.pos.xy - P0.pos.xy ) / Constants.scale;
    float d01 = length( R01 );
    R01 /= d01;
    if( d01 < P0.size + P1.size ) {
#if UPDATE_SPEED
        // The math for speed update is only valid for two-particle collisions.
        if( P0.numCollisions == 1 && P1.numCollisions == 1 ) {
            float v0 = dot( P0.speed.xy, R01 );
            float v1 = dot( P1.speed.xy, R01 );

            float m0 = P0.size * P0.size;
            float m1 = P1.size * P1.size;

            float new_v0 = ((m0 - m1) * v0 + 2.0 * m1 * v1) / (m0 + m1);
            P0.newSpeed += float3( (new_v0 - v0) * R01, 0 ); // TODO: compute for .z too
        }
#else
        {
            // Move the particle away
            P0.newPos += float3( -R01 * (P0.size + P1.size - d01) * Constants.scale * 0.51, 0 ); // TODO: use z

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
    
    int2 i2GridPos = GetGridLocation(Particle.pos.xy, Constants.gridSize).xy; // TODO: use 3D pos
    int GridWidth  = Constants.gridSize.x;
    int GridHeight = Constants.gridSize.y;

#if ! UPDATE_SPEED
    Particle.newPos       = Particle.pos;
    Particle.numCollisions = 0;
#else
    Particle.newSpeed     = Particle.speed;
    // Only update speed when there is single collision with another particle.
    if( Particle.numCollisions == 1 ) {
#endif
        for( int y = max( i2GridPos.y - 1, 0 ); y <= min( i2GridPos.y + 1, GridHeight - 1 ); ++y ) {
            for( int x = max( i2GridPos.x - 1, 0 ); x <= min(i2GridPos.x + 1, GridWidth - 1 ); ++x ) {
                int AnotherParticleIdx = ParticleListHead.Load(x + y * GridWidth);
                while( AnotherParticleIdx >= 0 ) {
                    if( iParticleIdx != AnotherParticleIdx ) {
                        ParticleAttribs AnotherParticle = Particles[AnotherParticleIdx];
                        CollideParticles( Particle, AnotherParticle );
                    }

                    AnotherParticleIdx = ParticleLists.Load(AnotherParticleIdx);
                }
            }
        }
#if UPDATE_SPEED
    }
    else if( Particle.numCollisions > 1 ) {
        // If there are multiple collisions, reverse the particle move direction to
        // avoid particle crowding.
        Particle.newSpeed = -Particle.speed;
    }
#else
    ClampParticlePosition( Particle.newPos, Particle.speed, Particle.size * Constants.scale );
#endif

    Particles[iParticleIdx] = Particle;
}
