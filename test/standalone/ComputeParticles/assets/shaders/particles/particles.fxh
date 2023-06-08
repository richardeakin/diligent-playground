
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

int grid3DTo1D( int3 loc, int3 gridSize )
{
    return loc.x + loc.y * gridSize.x + loc.z * gridSize.x * gridSize.y;
}

// returns 3D grid position in .xyz, flattened position in .w
int4 gridLocation( float3 pos, int3 gridSize )
{
    int3 loc;
    loc.x = clamp( int((pos.x + 1.0) * 0.5f * float(gridSize.x)), 0, gridSize.x - 1 );
    loc.y = clamp( int((pos.y + 1.0) * 0.5f * float(gridSize.y)), 0, gridSize.y - 1 );
    loc.z = clamp( int((pos.z + 1.0) * 0.5f * float(gridSize.z)), 0, gridSize.z - 1 );

    int flatLoc = grid3DTo1D( loc, gridSize );
    return int4( loc, flatLoc );
}
