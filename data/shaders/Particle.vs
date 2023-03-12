#version 110

attribute vec4 vertex; // <vec2 position, vec2 texCoords>
attribute float index;

varying vec2 TexCoords;
varying vec4 ParticleColor;

uniform vec2 offset[500];
uniform vec4 color[500];
uniform mat4 projection;

void main()
{
    float scale = 10.0;

    TexCoords = vertex.zw;
    ParticleColor = color[int(index)];

    gl_Position = projection * index* vec4((vertex.xy * scale) + offset[int(index)], 0.0, 1.0);
}
