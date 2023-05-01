#include "shaders/particles/structures.fxh"

cbuffer Constants {
    ParticleConstants Constants;
};

#ifndef THREAD_GROUP_SIZE
#   define THREAD_GROUP_SIZE 64
#endif

RWBuffer<int /*format=r32i*/> ParticleListHead;

[numthreads(THREAD_GROUP_SIZE, 1, 1)]
void main( uint3 Gid  : SV_GroupID,
           uint3 GTid : SV_GroupThreadID )
{
    // insure particle is within bounds, in case we are in the last dispatch group
    uint globalThreadIdx = Gid.x * uint(THREAD_GROUP_SIZE) + GTid.x;
    if( globalThreadIdx < Constants.numParticles ) {
        ParticleListHead[globalThreadIdx] = -1;
    }
}
