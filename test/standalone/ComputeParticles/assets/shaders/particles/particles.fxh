
// TODO: add a world bounds and use that for these methods so that locations are not limited to -1:1

void ClampParticlePosition( inout float3 pos,
                            inout float3 speed,
                            in    float  size )
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

    // TODO: clamp in Z
}

int3 GetGridLocation(float2 f2Pos, int2 i2ParticleGridSize)
{
    int3 i3GridPos;
    i3GridPos.x = clamp(int((f2Pos.x + 1.0) * 0.5 * float(i2ParticleGridSize.x)), 0, i2ParticleGridSize.x - 1);
    i3GridPos.y = clamp(int((f2Pos.y + 1.0) * 0.5 * float(i2ParticleGridSize.y)), 0, i2ParticleGridSize.y - 1);
    i3GridPos.z = i3GridPos.x + i3GridPos.y * i2ParticleGridSize.x;
    return i3GridPos;
}
