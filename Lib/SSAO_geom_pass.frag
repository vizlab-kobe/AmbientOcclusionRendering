#version 120
#include "qualifire.h"


// Input parameters from vertex shader.
FragIn vec4 color;
FragIn vec4 position;
FragIn vec3 normal;


void main()
{
    gl_FragData[0] = color;
    gl_FragData[1] = vec4( position.xyz, 1.0 );
    gl_FragData[2] = vec4( normal, 1.0 );
}
