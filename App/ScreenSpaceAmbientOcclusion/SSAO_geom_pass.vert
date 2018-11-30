#version 120
#include "qualifire.h"


// Output parameters to fragment shader.
VertOut vec4 color;
VertOut vec4 position;
VertOut vec3 normal;


void main()
{
    gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;

    color = gl_Color;
    position = gl_ModelViewMatrix * gl_Vertex;
    normal = normalize( gl_NormalMatrix * gl_Normal );
}
