struct Ray {
    float3 origin;
    float3 dir;
};

#define SDF_MAX_ITERATIONS 200
#define SDF_MIN_DIST 0.01
#define SDF_MAX_DIST 100.0    
#define DEBUG_SDF_GRADIENT 0
#define SIMPLE_SCENE 0

#define INTERSECT_FN sdf_intersect
//#define INTERSECT_FN sdf_intersectEnhanced

//! Information for the current object at this pixel
struct ObjectInfo {
    int id;                 // defines the intersected object
    float materialPart;     // define different parts of a material within an object 
    float3 pos;               // world-space
    float3 normal;
    float4 params0;
    //float3 velocity;
};

ObjectInfo initObjectInfo()
{
    ObjectInfo o;
    o.id = -1;
    o.materialPart = 0;
    o.pos = float3( 0, 0, 0 );
    o.normal = float3( 0, 1, 0 );
    o.params0 = float4( 0, 0, 0, 0 );

    return o;
}

// Information about a Ray-Marching intersection
struct IntersectInfo {
    float 	dist; //! SDF distance
    float 	rayLength; //! the distance from ray origin to the intersection
    int 	iterations; //! number of sphere-tracing iterations to the intersection
    bool 	isReflection; //! If this intersection is a reflection
};

float sdf_scene( in float3 p, inout ObjectInfo object, float3 worldMin, float3 worldMax );

// --------------------------------------------------------------------
// SDF object ids
// --------------------------------------------------------------------

static const int oid_nothing   = -1;
static const int oid_floor     = 1;
static const int oid_ball      = 2;
static const int oid_cone      = 3;
static const int oid_bbox      = 4;

// --------------------------------------------------------------------
// SDF object functions
// --------------------------------------------------------------------
// https://iquilezles.org/articles/distfunctions/

float sdSphere( float3 p, float s )
{
    return length( p ) - s;
}

float sdBox( float3 p, float3 b )
{
  float3 q = abs( p ) - b;
  return length( max( q, 0.0 ) ) + min( max( q.x, max( q.y,q.z ) ), 0.0 );
}

float sdPlane( float3 p, float3 n, float h )
{
    // n must be normalized
    return dot( p, n ) + h;
}

float sdCone( float3 p, float2 c, float h )
{
    // c is the sin/cos of the angle, h is height
    // Alternatively pass q instead of (c,h),
    // which is the point at the base in 2D
    float2 q = h * float2(c.x / c.y, -1.0);
    
    float2 w = float2(length(p.xz), p.y);
    float2 a = w - q * clamp(dot(w, q) / dot(q, q), 0.0, 1.0);
    float2 b = w - q * float2(clamp(w.x / q.x, 0.0, 1.0), 1.0);
    float k = sign(q.y);
    float d = min(dot(a, a), dot(b, b));
    float s = max(k * (w.x * q.y - w.y * q.x), k * (w.y - q.y));
    return sqrt(d) * sign(s);
}

float sdBoxFrame( float3 p, float3 b, float e )
{
	p = abs( p ) - b;
	float3 q = abs( p + e ) - e;
	return min( min(
		length( max( float3( p.x, q.y, q.z ), 0.0 ) ) + min( max( p.x, max( q.y, q.z ) ), 0.0 ),
		length( max( float3( q.x, p.y, q.z ), 0.0 ) ) + min( max( q.x, max( p.y, q.z ) ), 0.0 ) ),
		length( max( float3( q.x, q.y, p.z ), 0.0 ) ) + min( max( q.x, max( q.y, p.z ) ), 0.0 )
	);
}

// --- analytically box-filtered checkerboard ---
// http://iquilezles.org/www/articles/checkerfiltering/checkerfiltering.htm
float checkersGrad( in float2 p, in float2 ddx, in float2 ddy )
{
	// filter kernel
	float2 w = max( abs( ddx ), abs( ddy ) ) + 0.01;
	// analytical integral (box filter)
	float2 i = 2.0 * ( abs( frac( ( p - 0.5 * w ) / 2.0 ) - 0.5 ) - abs( frac( ( p + 0.5 * w ) / 2.0 ) - 0.5 ) ) / w;
	// xor pattern
	return 0.5 - 0.5 * i.x * i.y;
}

// x: smooth min, y: blending factor
// https://iquilezles.org/articles/smin/
float2 smin( float a, float b, float k )
{
	float h = max( k - abs( a - b ), 0.0 ) / k;
	float m = h * h * 0.5;
	float s = m * k * ( 1.0 / 2.0 );
	return ( a < b ) ? float2( a - s, m ) : float2( b - s, 1.0 - m );
}

// --------------------------------------------------------------------
// SDF Scene
// --------------------------------------------------------------------

float sdf_scene( in float3 p, inout ObjectInfo object, float3 worldMin, float3 worldMax )
{
    int id;
    float dist;
    float result = SDF_MAX_DIST + 1;

    //float floor = dot( p, float3( 0, 1, 0 ) ); // plane with normal pointing up

    // ground plane, normal pointing up
    float floor = sdPlane( p, normalize( float3( 0, 1, 0 ) ), 0 ); 
    result = floor;
    object.id = oid_floor;

    // do not do if check in order to smooth blend
#if SIMPLE_SCENE
    float3 ballCenter = float3( 0, 5.0, 0 );
    float ball = sdSphere( p - ballCenter, 5.0 );
    if( ball < result ) {
        result = ball;
        object.id = oid_ball;
        // object.materialPart = 0.5;
    }
#else
    float3 ballCenter = float3( 0, -3.0, 0 );
    float ball = sdSphere( p - ballCenter, 7.1 );

    float3 ballCenter2 = float3( 0, 4.1, 0 );
    float ball2 = sdSphere( p - ballCenter2, 1.5 );
    //ball = smin( ball, ball2, 2.1 );
    //result = ball;
    //if( ball < result ) {
        //result = ball;
    float2 s = smin( result, ball, 3.5 );
    result = s.x;
    object.id = oid_ball;
    object.materialPart = s.y;
    //}

    // three cones to make crude shape of a volcano
    // 1st cone (right side)
    float3 coneCenter = float3( 1.9, 7.5, -0.5 );
    float cone = sdCone( p - coneCenter, float2( 2.5, 6 ), 7.5 );
    //if( cone < result ) {
    //    result = cone;
    //    object.id = oid_cone;        
    //}
    s = smin( result, cone, 2.0 );
    result = s.x;
    object.id = oid_ball;
    //object.materialPart = s.y;

    // 2nd cone (left side)
    coneCenter = float3( -1.5, 6.7, -0.5 );
    cone = sdCone( p - coneCenter, float2( 3.7, 6 ), 7.5 );
    s = smin( result, cone, 1.0 );
    result = s.x;
    object.id = oid_ball;

    // 3rd cone
    coneCenter = float3( 0.4, 8.9, 0.5 );
    cone = sdCone( p - coneCenter, float2( 2.5, 6 ), 8.5 );
    s = smin( result, cone, 1.1 );
    result = s.x;
    object.id = oid_ball;
#endif    

    float3 boundsCenter = 0.5 * ( worldMin + worldMax );
    float3 boundsSize = 0.5 * ( worldMax - worldMin ); // from center to edge in each dimension
#if PHYSICS_SIM
    // invert box to act as a bounding box for physics 
    float bbox = -sdBox( p - boundsCenter, boundsSize );
#else
    float bbox = sdBoxFrame(p - boundsCenter, boundsSize, 0.03);
    bbox = 100; // disable
#endif
    // bbox = sdBox( p - boundsCenter, boundsSize );
    if( bbox < result ) {
        result = bbox;
        object.id = oid_bbox;
    }

    return result;
}

// --------------------------------------------------------------------
// Raymarching Functions
// --------------------------------------------------------------------

float3 sdf_calcNormal( in ObjectInfo object, float3 worldMin, float3 worldMax )
{
    float3 pos = object.pos;

    // precision of the normal computation
    // const float eps = 0.002;
    const float eps = 0.002;

    const float3 v1 = float3(  1.0, -1.0, -1.0 );
    const float3 v2 = float3( -1.0, -1.0,  1.0 );
    const float3 v3 = float3( -1.0,  1.0, -1.0 );
    const float3 v4 = float3(  1.0,  1.0,  1.0 );

    float3 N = v1 * sdf_scene( pos + v1 * eps, object, worldMin, worldMax ) 
             + v2 * sdf_scene( pos + v2 * eps, object, worldMin, worldMax )
             + v3 * sdf_scene( pos + v3 * eps, object, worldMin, worldMax ) 
             + v4 * sdf_scene( pos + v4 * eps, object, worldMin, worldMax );

#if DEBUG_SDF_GRADIENT
    return N;
#else
    return normalize( N );
#endif
}

IntersectInfo sdf_intersect( in Ray ray, inout ObjectInfo object, float3 worldMin, float3 worldMax )
{
    float scene = SDF_MIN_DIST * 2.0;
    float t = 0.0;
    float dist = -1.0; // TODO: why does this start at -1 (inside?)
    int i;
    for( i = 0; i < SDF_MAX_ITERATIONS; i++ ) {
        if( scene < SDF_MIN_DIST || t > SDF_MAX_DIST )
            break;

        object.pos = ray.origin + ray.dir * t;
        scene = sdf_scene( object.pos, object, worldMin, worldMax );
        t += scene;
    }

    if( t < SDF_MAX_DIST ) {
        dist = t;
        object.normal = sdf_calcNormal( object, worldMin, worldMax );
    }
    else {
        object.id = oid_nothing;
    }

    IntersectInfo result;
    result.dist = dist;
    result.rayLength = t;
    result.iterations = i;

    return result;
}


// Implementation of Enhanced Sphere Tracing algo from https://www.shadertoy.com/view/ldfyWs
IntersectInfo sdf_intersectEnhanced( in Ray ray, inout ObjectInfo object, float3 worldMin, float3 worldMax )
{
    float omega = 1.2;
    float t = SDF_MIN_DIST;
    float candidate_error = 1.0 / 0.0;
    float candidate_t = SDF_MIN_DIST;
    float previousRadius = 0.0;
    float stepLength = 0.0;
    float pixelRadius = 0.001; // TODO: need to calculate this?

    // TODO: check if this causes distortion at different aspect ratios. It might have to be min( res.x, res.y )
    //float pixelRadius       = 1.0 / uResolution.y;

    IntersectInfo result;
    result.iterations = 0;
    for( int i = 0; i < SDF_MAX_ITERATIONS; i++ ) {
        result.iterations += 1;
        object.pos = ray.origin + ray.dir * t;
        float signedRadius = sdf_scene( object.pos, object, worldMin, worldMax );
        float radius       = abs( signedRadius );

        bool sorFail = omega > 1.0 && ( radius + previousRadius ) < stepLength;
        if( sorFail ) {
            stepLength -= omega * stepLength;
            omega = 1.0;
        }
        else {
            stepLength = signedRadius * omega;
        }

        previousRadius = radius;

        float error = radius / t;

        if( ! sorFail && error < candidate_error ) {
            candidate_t     = t;
            candidate_error = error;
        }

        if( ! sorFail && error < pixelRadius || t > SDF_MAX_DIST )
            break;

        t += stepLength;
    }

    float intersection = -1;
    bool hit = t <= SDF_MAX_DIST && candidate_error <= pixelRadius;
    if( hit ) {
        object.pos = ray.origin + ray.dir * candidate_t;
        intersection = sdf_scene( object.pos, object, worldMin, worldMax );
        result.iterations += 1;

        const int discontinuityReductionIterations = 3;
        const float discontinuityReductionEpsilon  = 0.1;
        for( int i = 0; i < discontinuityReductionIterations; i++ ) {
            object.pos += ray.dir * ( intersection - discontinuityReductionEpsilon );
            intersection = sdf_scene( object.pos, object, worldMin, worldMax );
            result.iterations += 1;
        }

        object.normal = sdf_calcNormal( object, worldMin, worldMax );
    }
    else {
        object.id = oid_nothing;
    }

    result.dist = intersection;
    result.rayLength = t;

    return result;
}
