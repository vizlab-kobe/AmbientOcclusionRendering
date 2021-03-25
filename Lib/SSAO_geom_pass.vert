#version 120
#include "qualifire.h"

// Output parameters to fragment shader.
VertOut vec4 color;
VertOut vec4 position;
VertOut vec3 normal;

// Uniform variables (OpenGL variables).
uniform mat4 ModelViewMatrix; // model-view matrix
uniform mat4 ModelViewProjectionMatrix; // model-view projection matrix
uniform mat3 NormalMatrix; // normal matrix

void main()
{
    gl_Position = ModelViewProjectionMatrix * gl_Vertex;

    color = gl_Color;
    position = ModelViewMatrix * gl_Vertex; // in camera coordinate
    normal = normalize( NormalMatrix * gl_Normal ); // in camera coordinate
}
