#version 120

// Uniform variables (OpenGL variables).
uniform mat4 ModelViewProjectionMatrix;

void main()
{
//    gl_Position = ModelViewProjectionMatrix * gl_Vertex;
    gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;
    gl_TexCoord[0] = gl_MultiTexCoord0;
}
