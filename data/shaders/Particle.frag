#version 110

varying vec2 TexCoords;
varying vec4 ParticleColor;

uniform sampler2D sprite;

void main()
{
   gl_FragColor = (texture2D(sprite, TexCoords) * ParticleColor);
}  
