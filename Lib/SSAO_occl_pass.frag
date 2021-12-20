#version 120
#include "shading.h"
#include "texture.h"

// Uniform parameters.
uniform sampler2D color_texture;
uniform sampler2D position_texture;
uniform sampler2D normal_texture;
uniform sampler2D depth_texture;
uniform sampler1D kernel_texture;
uniform int kernel_size;
uniform ShadingParameter shading;

// Uniform variables (OpenGL variables).
uniform mat4 ProjectionMatrix;


float OcclusionFactor( vec4 position )
{
    int count = 0;
    float index = 0.0f;
    float dindex = 1.0f / float( kernel_size - 1 );
    for ( int i = 0; i < kernel_size ; i++, index += dindex )
    {
        vec3 p = LookupTexture1D( kernel_texture, index ).xyz;
        if ( p.x != 0 && p.y != 0 && p.z != 0 )
        {
            vec4 q = ProjectionMatrix * ( position + vec4( p, 0.0 ) );
            q = q * 0.5 / q.w + 0.5;
            if ( q.z < LookupTexture2D( depth_texture, q.xy ).z ) count++;
        }
    }

    return clamp( float( count )  * 2.0 / float( kernel_size ), 0.0, 1.0 );
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
    float occlusion = OcclusionFactor( position );

    // Shading.
#if   defined( ENABLE_LAMBERT_SHADING )
    vec3 shaded_color = SSAOShadingLambert( shading, color.rgb, L, N, occlusion );

#elif defined( ENABLE_PHONG_SHADING )
    vec3 V = normalize( -position.xyz );
    vec3 shaded_color = SSAOShadingPhong( shading, color.rgb, L, N, V, occlusion );

#elif defined( ENABLE_BLINN_PHONG_SHADING )
    vec3 V = normalize( -position.xyz );
    vec3 shaded_color = SSAOShadingBlinnPhong( shading, color.rgb, L, N, V, occlusion );

#else // DISABLE SHADING
    vec3 shaded_color = ShadingNone( shading, color.rgb * occlusion );
#endif

#if defined( ENABLE_DRAWING_OCCLUSION_FACTOR )
    // Draw occlusion factor as a fragment color
    gl_FragColor = vec4( occlusion, occlusion, occlusion, 1.0 );
#else
    gl_FragColor = vec4( shaded_color, 1.0 );
#endif

    gl_FragDepth = LookupTexture2D( depth_texture, gl_TexCoord[0].st ).z;
}
