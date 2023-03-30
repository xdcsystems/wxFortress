#version 120

// Input vertex data, different for all executions of this shader.
attribute vec2 vertex;
attribute vec2 vertexUV;

// Output data ; will be interpolated for each fragment.
varying vec2 UV;
uniform mat4 model;
uniform mat4 projection;

void main()
{
  gl_Position = projection * vec4( vertex, 0, 1 );
  UV = vertexUV;
}

