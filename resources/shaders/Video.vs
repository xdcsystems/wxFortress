#version 110

attribute vec2 aPos;
attribute vec2 aTexCoord;

varying vec2 TexCoord;

uniform vec2 uScale;

void main()
{
    gl_Position = vec4( aPos * uScale, 0.0, 1.0 );
    TexCoord = aTexCoord;
}