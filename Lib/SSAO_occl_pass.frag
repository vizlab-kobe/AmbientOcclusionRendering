#version 120
#include "shading.h"
#include "texture.h"

// Uniform parameters.
uniform sampler2D color_texture;
uniform sampler2D position_texture;
uniform sampler2D normal_texture;
uniform sampler2D depth_texture;
uniform sampler1D kernel_texture;
uniform sampler2D noise_texture;
uniform int kernel_size;
uniform float kernel_radius;
uniform float kernel_bias;
uniform float intensity;
uniform vec2 noise_scale;

uniform ShadingParameter shading;

// Uniform variables (OpenGL variables).
uniform mat4 ProjectionMatrix;


float OcclusionFactor( vec4 position, mat3 tbn )
{
    float occlusion = 0.0;
    float index = 0.0f;
    float dindex = 1.0f / float( kernel_size );
    for ( int i = 0; i < kernel_size ; i++, index += dindex )
    {
        vec3 p = tbn * LookupTexture1D( kernel_texture, index ).xyz;
        p = p * kernel_radius + position.xyz;

        vec4 q = ProjectionMatrix * vec4( p, 1.0 );
        q.xyz /= q.w;
        q.xyz = q.xyz * 0.5 + 0.5; // to clip coord.

        float depth = LookupTexture2D( depth_texture, q.xy ).z;
        float range_check = 1.0 - smoothstep( 0.0, 1.0, kernel_radius / abs( p.z - depth ) );
        occlusion += ( q.z - kernel_bias >= depth ? 1.0 : 0.0 ) * range_check;
    }

    return 1.0 - occlusion / kernel_size;
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
    vec3 random_vec = LookupTexture2D( noise_texture, gl_TexCoord[0].st * noise_scale ).xyz;
    vec3 tangent = normalize( random_vec - normal * dot( random_vec, normal ) );
    vec3 bitangent = cross( normal, tangent );
    mat3 tbn = mat3( tangent, bitangent, normal );

    float occlusion = OcclusionFactor( position, tbn );
    occlusion = clamp( pow( occlusion, intensity ), 0.0, 1.0 );

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
    gl_FragColor = vec4( vec3( occlusion ), 1.0 );
#else
    gl_FragColor = vec4( shaded_color, 1.0 );
#endif

    gl_FragDepth = LookupTexture2D( depth_texture, gl_TexCoord[0].st ).z;
}
