
void ClampParticlePosition( inout float3 pos, inout float3 speed, in float size, in float3 worldMin, in float3 worldMax )
{
    if( speed.x > 0.0 && pos.x > worldMax.x ) {
        speed.x *= -1.0;
    }
    if( speed.x < 0.0 && pos.x < worldMin.x ) {
        speed.x *= -1.0;
    }

    if( speed.y > 0.0 && pos.y > worldMax.y ) {
        speed.y *= -1.0;
    }
    if( speed.y < 0.0 && pos.y < worldMin.y ) {
        speed.y *= -1.0;
    }

    if( speed.z > 0.0 && pos.z > worldMax.z ) {
        speed.z *= -1.0;
    }
    if( speed.z < 0.0 && pos.z < worldMin.z ) {
        speed.z *= -1.0;
    }
}

int Grid3DTo1D( in int3 loc, in int3 gridSize )
{
    return loc.x + loc.y * gridSize.x + loc.z * gridSize.x * gridSize.y;
}

// TODO: cleanup
#if 1
// returns 3D grid position in .xyz, flattened position in .w
int4 GetGridLocation( float3 pos, int3 gridSize )
{
    int4 loc;
    loc.x = clamp(int((pos.x + 1.0) * 0.5 * float(gridSize.x)), 0, gridSize.x - 1);
    loc.y = clamp(int((pos.y + 1.0) * 0.5 * float(gridSize.y)), 0, gridSize.y - 1);
    loc.z = clamp(int((pos.z + 1.0) * 0.5 * float(gridSize.z)), 0, gridSize.z - 1);

    loc.w = Grid3DTo1D( loc.xyz, Grid3DTo1D( loc.xyz, gridSize ) );
    return loc;
}

#else
// Original
int3 GetGridLocation( float2 f2Pos, int2 i2ParticleGridSize )
{
    int3 i3GridPos;
    i3GridPos.x = clamp(int((f2Pos.x + 1.0) * 0.5 * float(i2ParticleGridSize.x)), 0, i2ParticleGridSize.x - 1);
    i3GridPos.y = clamp(int((f2Pos.y + 1.0) * 0.5 * float(i2ParticleGridSize.y)), 0, i2ParticleGridSize.y - 1);
    i3GridPos.z = i3GridPos.x + i3GridPos.y * i2ParticleGridSize.x;
    return i3GridPos;
}
#endif