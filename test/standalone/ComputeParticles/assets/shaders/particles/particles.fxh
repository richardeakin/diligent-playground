
// TODO: add a world bounds and use that for these methods so that locations are not limited to -1:1
void ClampParticlePosition( inout float3 pos, inout float3 speed, in float size )
{
    if( pos.x + size > 1.0 ) {
        pos.x -= pos.x + size - 1.0;
        speed.x *= -1.0;
    }
    if( pos.x - size < -1.0 ) {
        pos.x += -1.0 - ( pos.x - size );
        speed.x *= -1.0;
    }

    if( pos.y + size > 1.0 ) {
        pos.y -= pos.y + size - 1.0;
        speed.y *= -1.0;
    }
    if( pos.y - size < -1.0 ) {
        pos.y += -1.0 - ( pos.y - size );
        speed.y *= -1.0;
    }

    if( pos.z + size > 1.0 ) {
        pos.z -= pos.z + size - 1.0;
        speed.z *= -1.0;
    }
    if( pos.z - size < -1.0 ) {
        pos.z += -1.0 - ( pos.z - size );
        speed.z *= -1.0;
    }
}

#if 1
// TODO NEXT: update for a 3D grid position
// returns 3D grid position in .xyz, flattened position in .w
int4 GetGridLocation( float3 pos, int3 gridSize )
{
    int4 gridPos;
    gridPos.x = clamp(int((pos.x + 1.0) * 0.5 * float(gridSize.x)), 0, gridSize.x - 1);
    gridPos.y = clamp(int((pos.y + 1.0) * 0.5 * float(gridSize.y)), 0, gridSize.y - 1);
    gridPos.z = clamp(int((pos.z + 1.0) * 0.5 * float(gridSize.z)), 0, gridSize.z - 1);

    // TODO NEXT: figure out what Z is used here for before converting to 3D
    gridPos.w = gridPos.x + gridPos.y * gridSize.x + gridPos.z * gridSize.x * gridSize.y;
    return gridPos;
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