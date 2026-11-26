#version 110

attribute vec4 vertex; // <vec2 worldPos, vec2 texCoords>
attribute vec4 color;

varying vec2 TexCoords;
varying vec4 ParticleColor;

uniform mat4 projection;

void main()
{
    TexCoords = vertex.zw;
    ParticleColor = color;

    gl_Position = projection * vec4( vertex.xy, 0.0, 1.0 );
}
