#version 120
#include "shading.h"
#include "texture.h"


// Uniform parameters.
uniform sampler2D color_texture;
uniform sampler2D position_texture;
uniform sampler2D normal_texture;
uniform sampler2D depth_texture;
uniform ShadingParameter shading;

// Uniform variables (OpenGL variables).
uniform mat4 ProjectionMatrix;

//Generate random value from -1.0 to 1.0
float rand(vec2 co){
    return fract(sin(dot(co.xy ,vec2(12.9898,78.233))) * 43758.5453);
}

float OcclusionFactor( vec4 position, int nsamples )
{
/*
    //Hexahedral lattice type sampling
    float l = 0.4 / float( nsamples );
    
    int count = 0;
    for ( int x = -nsamples; x <= nsamples; x++ )
    {
        for ( int y = -nsamples; y <= nsamples; y++ )
        {
            for ( int z = -nsamples; z <= nsamples; z++ )
            {
                if ( x != 0 && y != 0 && z != 0 )
                {
                    vec4 q = ProjectionMatrix * ( position + l * vec4( float(x), float(y), float(z), 0.0 ) );
                    q = q * 0.5 / q.w + 0.5;
                    if ( q.z < LookupTexture2D( depth_texture, q.xy ).z ) count++;
                }
            }
        }
    }
    
    int n = ( 2 * nsamples + 1 ) * ( 2 * nsamples + 1 ) * ( 2 * nsamples + 1 ) - 1;
    return clamp( float( count ) * 3.0 / float( n ), 0.0, 1.0 );
*/

    const float pi = 3.1415926;
    float l = 0.4;
    int count = 0;

    for ( int i = 0; i < nsamples ; i++ )
    {
	float r = rand( vec2( i, i ) ) * l * 0.5 + 0.5;
	float u = rand( vec2( i + nsamples, i )) * 0.5 + 0.5;
	float v = rand( vec2( i, i + nsamples ));

	float R = r * r;

	float x = R * sqrt( 1.0 - v * v ) * cos( 2 * pi * u );
	float y = R * sqrt( 1.0 - v * v ) * sin( 2 * pi * u );
	float z = R * v;

	vec3 K = vec3( x, y, z );

	if ( x != 0 && y != 0 && z != 0 )
	{
    	vec4 q = ProjectionMatrix * ( position + vec4( K, 0.0 ) );
	q = q * 0.5 / q.w + 0.5;
	if ( q.z < LookupTexture2D( depth_texture, q.xy ).z ) count++;
	}
    }
    
    return clamp( float( count ) * 2.0 / float( nsamples ), 0.0, 1.0 );
}

void main()
{
    vec4 color = LookupTexture2D( color_texture, gl_TexCoord[0].st );
    if ( color.a == 0.0 ) { discard; return; }

    vec4 position = LookupTexture2D( position_texture, gl_TexCoord[0].st );
    vec3 normal = LookupTexture2D( normal_texture, gl_TexCoord[0].st ).xyz;

    // Light position in camera coordinate.
    vec3 light_position = gl_LightSource[0].position.xyz;

    // Light vector (L) and Normal vector (N) in camera coordinate.
    vec3 L = normalize( light_position - position.xyz );
    vec3 N = normalize( normal );

    // Ambient occlusion.
    int nsamples = 256;
    float occlusion = OcclusionFactor( position, nsamples );
    
    // Shading.
#if   defined( ENABLE_LAMBERT_SHADING )
    vec3 shaded_color = ShadingLambert( shading, color.rgb * occlusion, L, N );

#elif defined( ENABLE_PHONG_SHADING )
    vec3 V = normalize( -position.xyz );
    vec3 shaded_color = ShadingPhong( shading, color.rgb * occlusion, L, N, V );

#elif defined( ENABLE_BLINN_PHONG_SHADING )
    vec3 V = normalize( -position.xyz );
    vec3 shaded_color = ShadingBlinnPhong( shading, color.rgb * occlusion, L, N, V );

#else // DISABLE SHADING
    vec3 shaded_color = ShadingNone( shading, color.rgb * occlusion );
#endif

    gl_FragColor = vec4( shaded_color, 1.0 );
    gl_FragDepth = LookupTexture2D( depth_texture, gl_TexCoord[0].st ).z;
}
