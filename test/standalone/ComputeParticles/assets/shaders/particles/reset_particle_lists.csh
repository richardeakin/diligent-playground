#include "shaders/particles/structures.fxh"
#line 1

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
    uint uiGlobalThreadIdx = Gid.x * uint(THREAD_GROUP_SIZE) + GTid.x;
    if( uiGlobalThreadIdx < uint( Constants.gridSize.x * Constants.gridSize.y ) ) {
        ParticleListHead[uiGlobalThreadIdx] = -1;
    }
}
