#include "shaders/particles/structures.fxh"
#include "shaders/particles/particles.fxh"

#define PHYSICS_SIM 1
#include "shaders/canvas/sdfScene.fxh"

// 0: disabled, 1: only consider this particle's bin, 2: also consider neighboring bins
// FIXME: 2 is still broken
// TODO: add mode to only consider lateral neighbords, which is much less than mode 2
// - this should help tell if 2 is actually broken or just taking too long
#define BINNING_MODE 0
#define PARTICLES_AVOID_SDF 1

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
        p0.numInteractions += 1;
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

// TODO: want to cast a ray and see how close we are to something in the scene ahead of us
// - wasn't working at first try so I switched to using sdf_scene() + sdf_calcNorma()
// - this allows movement but likely innacurate / difficult to control
void interactScene( inout ParticleAttribs p )
{
    Ray ray;
    ray.origin = p.pos;
    ray.dir = normalize( p.vel );

    ObjectInfo object = initObjectInfo();
    IntersectInfo intersect = INTERSECT_FN(ray, object, Constants.worldMin, Constants.worldMax);
    p.distToSDF = intersect.dist;
    //p.distToSDF = sdf_scene( p.pos, object, Constants.worldMin, Constants.worldMax );
    
    const float distToTurn = 5.0;
    if( p.distToSDF < distToTurn ) {
        p.nearestSDFObject = object.id;
        //float3 N = sdf_calcNormal( object, Constants.worldMin, Constants.worldMax );
        float N = object.normal;
        float strength = distToTurn - p.distToSDF;
        strength *= 3.0;
        p.accel += N * strength;
        p.sdfClosestNormal = N;
        p.sdfRepelStrength = strength;
    }
    else {
        p.nearestSDFObject = 0;
        p.sdfClosestNormal = float3( 0, 0, 0 );
        p.sdfRepelStrength = 0;
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
    particle.numInteractions  = 0;
    
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

#if PARTICLES_AVOID_SDF
    interactScene( particle );
#endif

    particle.newVel += particle.accel * Constants.deltaTime;

    Particles[particleId] = particle;
}
