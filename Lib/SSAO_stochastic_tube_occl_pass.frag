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


float OcclusionFactor( vec4 position, int resolution )
{
    float l = 0.4 / float( resolution );

    int count = 0;
    for ( int x = -resolution; x <= resolution; x++ )
    {
        for ( int y = -resolution; y <= resolution; y++ )
        {
            for ( int z = -resolution; z <= resolution; z++ )
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

    int n = ( 2 * resolution + 1 ) * ( 2 * resolution + 1 ) * ( 2 * resolution + 1 ) - 1;
    return clamp( float( count ) * 3.0 / float( n ), 0.0, 1.0 );
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
    int resolution = 3;
    float occlusion = OcclusionFactor( position, resolution );

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