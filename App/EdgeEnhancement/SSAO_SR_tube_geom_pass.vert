/*****************************************************************************/
/**
 *  @file   shader.vert
 *  @author Naohisa Sakamoto
 */
/*----------------------------------------------------------------------------
 *
 *  Copyright 2007 Visualization Laboratory, Kyoto University.
 *  All rights reserved.
 *  See http://www.viz.media.kyoto-u.ac.jp/kvs/copyright/ for details.
 *
 *  $Id: shading.vert 476 2010-02-14 15:45:46Z naohisa.sakamoto $
 */
/*****************************************************************************/
#version 120
#include "qualifire.h"
#include "texture.h"

// Input parameters.
VertIn vec2 random_index; // index for accessing to the random texture
VertIn float value; // normalized scalar value for the vertex

// Output parameters to fragment shader.
VertOut vec4 position;

VertOut vec4 diffuse;
VertOut vec3 x_i;
VertOut vec3 y_i;
VertOut vec3 coord;
VertOut vec2 depth0;
VertOut vec2 depth1;

VertOut vec2 index; // index for accessing to the random texture

// Uniform variables (OpenGL variables).
uniform mat4 ModelViewMatrix; // model-view matrix
uniform mat4 ProjectionMatrix; // projection matrix
uniform mat3 NormalMatrix; // normal matrix
uniform sampler1D transfer_function_texture; // transfer function texture
uniform float min_value;
uniform float max_value;

/*===========================================================================*/
/**
 *  @brief  Main function of vertex shader.
 */
/*===========================================================================*/
void main()
{
    vec4 p = ModelViewMatrix * gl_Vertex; // vertex in world coordinate
    vec3 v = normalize( -p.xyz ); // vector from camera to the vertex in world coordinate
    vec3 t = NormalMatrix * gl_Normal; // tangent in world coordinate
	
    position = p;
	vec3 u_vertical = -p.xyz;

    // Side and up extrusion vectors. 
    y_i = normalize( cross( u_vertical, t ) );
    x_i = normalize( cross( y_i, t ) );

    // Extrude the vertex.
    p.xyz += y_i * gl_MultiTexCoord0.x;
    gl_Position = ProjectionMatrix * p;

    float tfunc_index = ( value - min_value ) / ( max_value - min_value );
    gl_FrontColor = LookupTexture1D( transfer_function_texture, tfunc_index );

    diffuse = gl_FrontColor;
    coord = gl_MultiTexCoord0.xzy;
    depth0 = gl_Position.zw;

    float depth = min( 1.0 / dot( x_i, -v ), 1.0) * abs( gl_MultiTexCoord0.x );
    vec4 mp = ProjectionMatrix * vec4( p.xyz + v * depth, 1.0 );

    // transfer front position z and w values to fragment shader
    depth1 = mp.zw;

    // modify upvec in tangent direction
    x_i += t * gl_MultiTexCoord0.w;

    index = random_index;
}
