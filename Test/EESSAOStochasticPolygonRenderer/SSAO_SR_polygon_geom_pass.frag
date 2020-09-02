/*****************************************************************************/
/**
 *  @file   polygon.frag
 *  @author Jun Nishimura, Naohisa Sakamoto
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
#include "qualifire.h"
#include "texture.h"


// Input parameters from vertex shader
FragIn vec3 position; // vertex position in camera coordinate
FragIn vec3 normal; // normal vector in camera coodinate
FragIn vec2 index; // index for accessing to the random texture

// Uniform parameters.
uniform sampler2D random_texture; // random texture to generate random number
uniform float random_texture_size_inv; // reciprocal value of the random texture size
uniform vec2 random_offset; // offset values for accessing to the random texture
uniform ShadingParameter shading; // shading parameters


/*===========================================================================*/
/**
 *  @brief  Returns random index.
 *  @param  p [in] pixel coordinate value
 *  @return random index as 2d vector
 */
/*===========================================================================*/
vec2 RandomIndex( in vec2 p )
{
    float x = float( int( index.x ) * 73 );
    float y = float( int( index.y ) * 31 );
    return ( vec2( x, y ) + random_offset + p ) * random_texture_size_inv;
}

/*===========================================================================*/
/**
 *  @brief  Main function of fragment shader.
 */
/*===========================================================================*/
void main()
{
    vec3 color = gl_Color.rgb;
    float alpha = gl_Color.a;
    if ( alpha == 0.0 ) { discard; return; }

    vec3 E = normalize( -position.xyz );
    vec3 N = normalize( normal );
    float NE = dot( N, E );
    //if ( NE > 0.0 )
    {
        alpha = pow( alpha, abs( NE ) );
    }
    //else
    //{
    //    alpha = alpha * ( 1.0 - abs( NE ) );
    //}

    float R = LookupTexture2D( random_texture, RandomIndex( gl_FragCoord.xy ) ).a;
    if ( R > alpha ) { discard; return; }

    gl_FragData[0] = vec4( color, alpha );
    gl_FragData[1] = vec4( position.xyz, 1.0 );
    gl_FragData[2] = vec4( normal, 1.0 );
}
