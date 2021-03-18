#version 120
#include "shading.h"
#include "texture.h"

// NUMBER_OF_SAMPLING_POINTS is set from SSAO renderer.

// Uniform parameters.
uniform sampler2D color_texture;
uniform sampler2D position_texture;
uniform sampler2D normal_texture;
uniform sampler2D depth_texture;
uniform vec3 sampling_points[ NUMBER_OF_SAMPLING_POINTS ];
uniform ShadingParameter shading;

// Uniform variables (OpenGL variables).
uniform mat4 ProjectionMatrix;


float OcclusionFactor( vec4 position, int nsamples )
{
    if( nsamples == 1 ) return 1.0;
    int count = 0;
    for ( int i = 0; i < nsamples ; i++ )
    {
        vec3 p = sampling_points[i];
        if ( p.x != 0 && p.y != 0 && p.z != 0 )
        {
            vec4 q = ProjectionMatrix * ( position + vec4( p, 0.0 ) );
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
    int nsamples = NUMBER_OF_SAMPLING_POINTS;
    float occlusion = OcclusionFactor( position, nsamples );

    // Shading.
#if   defined( ENABLE_LAMBERT_SHADING )
    vec3 shaded_color = SSAOShadingLambert( shading, color.rgb, L, N, occlusion  );

#elif defined( ENABLE_PHONG_SHADING )
    vec3 V = normalize( -position.xyz );
    vec3 shaded_color = SSAOShadingPhong( shading, color.rgb, L, N, V, occlusion );

#elif defined( ENABLE_BLINN_PHONG_SHADING )
    vec3 V = normalize( -position.xyz );
    vec3 shaded_color = SSAOShadingBlinnPhong( shading, color.rgb, L, N, V, occlusion );

#else // DISABLE SHADING
    vec3 shaded_color = ShadingNone( shading, color.rgb * occlusion );
#endif

    //gl_FragColor = vec4( shaded_color, 1.0 );
    gl_FragColor = vec4( vec3( occlusion ), 1.0 );
    gl_FragDepth = LookupTexture2D( depth_texture, gl_TexCoord[0].st ).z;
}
