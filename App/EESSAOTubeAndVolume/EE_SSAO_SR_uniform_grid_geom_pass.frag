/*****************************************************************************/
/**
 *  @file   uniform_grid.frag
 *  @author Naohisa Sakamoto, Naoya Maeda
 */
/*----------------------------------------------------------------------------
 *
 *  Copyright (c) Visualization Laboratory, Kyoto University.
 *  All rights reserved.
 *  See http://www.viz.media.kyoto-u.ac.jp/kvs/copyright/ for details.
 *
 *  $Id$
 */
/*****************************************************************************/
#version 120
#include "shading.h"
#include "volume.h"
#include "transfer_function.h"
#include "qualifire.h"
#include "texture.h"


// Input parameters.
FragIn vec3 position_ndc;

// Uniform parameters.
uniform sampler2D entry_points; // entry points (front face)
uniform sampler2D exit_points; // exit points (back face)
uniform float sampling_step; // sampling step
uniform VolumeParameter volume; // volume parameter
uniform sampler3D volume_data; // volume data
uniform ShadingParameter shading; // shading parameter
uniform TransferFunctionParameter transfer_function; // transfer function
uniform sampler1D transfer_function_data; // 1D transfer function data
uniform float width; // screen width
uniform float height; // screen height
uniform float to_zw1; // scaling parameter: (f*n)/(f-n)
uniform float to_zw2; // scaling parameter: 0.5*((f+n)/(f-n))+0.5
uniform float to_ze1; // scaling parameter: 0.5 + 0.5*((f+n)/(f-n))
uniform float to_ze2; // scaling parameter: (f-n)/(f*n)
uniform sampler2D random_texture; // random texture to generate random number
uniform float random_texture_size_inv; // reciprocal value of the random texture size
uniform vec2 random_offset; // offset values for accessing to the random texture
uniform float edge_factor;

// Uniform variables (OpenGL variables).
uniform mat4 ModelViewProjectionMatrixInverse; // inverse matrix of model-view projection matrix
uniform mat4 ModelViewMatrix; // model-view matrix
uniform mat3 NormalMatrix; // normal matrix


/*===========================================================================*/
/**
 *  @brief  Returns random index.
 *  @param  p [in] pixel coordinate value
 *  @return random index as 2d vector
 */
/*===========================================================================*/
vec2 RandomIndex( in vec2 p )
{
    return ( random_offset + p ) * random_texture_size_inv;
}

/*===========================================================================*/
/**
 *  @brief  Return ray depth in window coordinate.
 *  @param  t [in] ratio parameter [0-1]
 *  @param  entry_depth [in] depth at entry point in window coodinate
 *  @param  exit_depth [in] depth at exit point in window coordinate
 *  @return depth in window coordinate
 */
/*===========================================================================*/
float RayDepth( in float t, in float entry_depth, in float exit_depth )
{
    // Calculate the depth value in window coordinate.
    // See: http://www.opengl.org/resources/faq/technical/depthbuffer.htm

    // zw_front: depth value at the entry point in window coordinate
    // ze_front: depth value at the entry point in camera (eye) coordinate
    float zw_front = entry_depth;
    float ze_front = 1.0 / ((zw_front - to_ze1)*to_ze2);

    // zw_front: depth value at the exit point in window coordinate
    // ze_front: depth value at the exit point in camera (eye) coordinate
    float zw_back = exit_depth;
    float ze_back = 1.0 / ((zw_back - to_ze1)*to_ze2);

    // First, the depth value at the dividing point (ze_current) in
    // camera coordinate is calculated with linear interpolation.
    // And then, the depth value in window coordinate (zw_current) is
    // converted from ze_current.
    float ze_current = ze_front + t * (ze_back - ze_front);
    float zw_current = (1.0/ze_current)*to_zw1 + to_zw2;

    return zw_current;
}

/*===========================================================================*/
/**
 *  @brief  Returns a transformed object coordinate from normalized device coordinate.
 *  @param  ndc [in] coordinate in normalized device coordinate
 *  @return coordinate in object coordinate
 */
/*===========================================================================*/
vec3 NDC2Obj( const in vec3 ndc )
{
    vec4 temp = ModelViewProjectionMatrixInverse * vec4( ndc, 1.0 );
    return temp.xyz / temp.w;
}

/*===========================================================================*/
/**
 *  @brief  Main function of fragment shader.
 */
/*===========================================================================*/
void main()
{
    vec2 index = vec2( gl_FragCoord.x / width, gl_FragCoord.y / height );
    vec4 entry = LookupTexture2D( entry_points, index );
    vec4 exit = LookupTexture2D( exit_points, index );

    // Entry and exit points and its depth values.
    vec3 entry_point = ( volume.resolution - vec3(1.0) ) * entry.xyz;
    vec3 exit_point = ( volume.resolution - vec3(1.0) ) * exit.xyz;
    if ( entry_point == exit_point ) { discard; return; } // out of volume

    float entry_depth = entry.w;
    float exit_depth = exit.w;

    // Front face clipping.
    if ( entry_depth == 1.0 )
    {
        entry_point = NDC2Obj( vec3( position_ndc.xy, -1.0 ) );
        entry_depth = 0.0;
    }

    // Number of steps (segments) along the viewing ray.
    float segment = distance( exit_point, entry_point );
#if defined( ENABLE_ALPHA_CORRECTION )
    int nsteps = 300;
    float dt = segment / float( nsteps );
    float dT = dt / sampling_step;
#else
    float dt = sampling_step;
    int nsteps = int( floor( segment / dt ) );
#endif

    // Ray direction.
    vec3 direction = dt * normalize( exit_point - entry_point );

    float tfunc_scale = 1.0 / ( transfer_function.max_value - transfer_function.min_value );

    // Random number.
    float R = LookupTexture2D( random_texture, RandomIndex( gl_FragCoord.xy ) ).a;

    // Ray traversal.
    float accum_alpha = 0.0;
    vec3 position = entry_point;
    float w = 0.0;
    float dd = dt / segment;
    for ( int i = 0; i < nsteps; i++, w += dd )
    {
        // Get the scalar value from the 3D texture.
        // NOTE: The volume index which is a index to access the volume data
        // represented as 3D texture can be calculate as follows:
        //
        //     vec3 A = ( R - vec3(1.0) ) / R; // ajusting parameter
        //     vec3 I = vec3( P + vec3(0.5) ) * A / ( R - vec3(1.0) );
        //            = vec3( P + vec3(0.5) ) / R;
        //
        // where, I: volume index, P: sampling point, R: volume resolution.
        vec3 volume_index = vec3( ( position + vec3(0.5) ) / volume.resolution );
        vec4 value = LookupTexture3D( volume_data, volume_index );
        float scalar = mix( volume.min_range, volume.max_range, value.w );

        // Get the source color from the transfer function.
        float tfunc_index = ( scalar - transfer_function.min_value ) * tfunc_scale;
        vec4 c = LookupTexture1D( transfer_function_data, tfunc_index );

#if defined( ENABLE_ALPHA_CORRECTION )
        c.a = 1.0 - pow( 1.0 - c.a, dT );
#endif
        // Get the normal vector in object coordinate.
        vec3 offset_index = vec3( volume.resolution_reciprocal );
        vec3 normal = VolumeGradient( volume_data, volume_index, offset_index );

        // Normal vector (N) in object coordinate.
        vec3 N = normalize( normal );

        // Edge enhancement
        if ( edge_factor > 0.0 )
        {
            if ( length( N ) != 0.0 )
            {
                vec3 E = normalize( -direction );
                float dotNE = abs( dot( N, E ) );
                if ( dotNE > 0.0 )
                {
                    c.a = min( 1.0, c.a / pow( dotNE, edge_factor ) );
                }
            }
        }

        // Stochastic color assignment
        accum_alpha += ( 1.0 - accum_alpha ) * c.a;
        if ( R <= accum_alpha )
        {
            gl_FragData[0] = vec4( c.rgb, 1.0 );
            gl_FragData[1] = ModelViewMatrix * vec4( position, 1.0 ); // position in camera coordinate
            gl_FragData[2] = vec4( NormalMatrix * N, 1.0 ); // normal vector in camera coordinate
            gl_FragDepth = RayDepth( w, entry_depth, exit_depth );
            return;
        }

        position += direction;
    }

    discard;
}
