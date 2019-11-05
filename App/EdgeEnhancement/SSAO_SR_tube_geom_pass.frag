/*****************************************************************************/
/**
 *  @file   shader.frag
 *  @author Naohisa Sakamoto
 */
/*----------------------------------------------------------------------------
 *
 *  Copyright 2007 Visualization Laboratory, Kyoto University.
 *  All rights reserved.
 *  See http://www.viz.media.kyoto-u.ac.jp/kvs/copyright/ for details.
 *
 *  $Id: shading.frag 485 2010-03-09 05:44:44Z naohisa.sakamoto $
 */
/*****************************************************************************/
#version 120
#include "shading.h"
#include "qualifire.h"
#include "texture.h"

// Input parameters from vertex shader.
FragIn vec4 position;

FragIn vec4 diffuse;
FragIn vec3 x_i;
FragIn vec3 y_i;
FragIn vec3 coord;
FragIn vec2 depth0;
FragIn vec2 depth1;

FragIn vec2 index; // index for accessing to the random texture

// Uniform parameters.
uniform ShadingParameter shading;
uniform float line_width;
uniform sampler2D shape_texture; // x = 0.0 ~ 1.0, y = root(1.0-t^2), z = root(1.0-t^2), t = -1.0 ~ 1.0
uniform sampler2D diffuse_texture; // All 255
uniform sampler2D random_texture; // random texture to generate random number
uniform float random_texture_size_inv; // reciprocal value of the random texture size
uniform vec2 random_offset; // offset values for accessing to the random texture
uniform mat4 ProjectionMatrix;
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
    float opacity = diffuse.a;
    if ( opacity == 0.0 ) { discard; return; }

    // Stochastic color assignment.
    float R = LookupTexture2D( random_texture, RandomIndex( gl_FragCoord.xy ) ).a;
	// Judge drawing this fragment.
    if ( R > opacity ) { discard; return; }

    vec2 tcd = coord.xy;  // tcd is Texture coordinate 
    tcd.x = ( coord.x / coord.z ) * 0.5 + 0.5; // Convert to clipping space

    vec3 tex = LookupTexture2D( shape_texture, tcd.xy ).xyz;
    tex.x = tex.x*2.0 - 1.0; // x = -1.0 ~ 1.0

    vec4 color;
    if ( tcd.x < 0.0 || tcd.x > 1.0 )
    {
        color = vec4( 0.0, 0.0, 0.0, 0.0 );
    }
    else
    {
        color = diffuse * LookupTexture2D( diffuse_texture, tcd.xy );
    }

    vec3 normal = y_i * tex.x - tex.y * x_i; // y_i is up vector, x_i is depth vector
	
	vec4 v = ProjectionMatrix * vec4(-position.xyz, position.w);
	float edge = 1.0 - clamp(abs(dot(normalize(normal),normalize(v.xyz))), 0.0, 0.8);
	color.a = color.a * edge;
	if( R > color.a ) { discard; return; }

    gl_FragData[0] = color;
    gl_FragData[1] = vec4( position.xyz, 1.0 );
    gl_FragData[2] = vec4( normal, 1.0 );

    vec2 rdep = tex.y * depth1 + ( 1.0 - tex.y ) * depth0;
    gl_FragDepth = ( rdep.x / rdep.y ) * 0.5 + 0.5;
}
