#version 110

// Interpolated values from the vertex shaders
varying vec2 UV;

// Values that stay constant for the whole mesh.
uniform sampler2D charImage;

uniform vec3 textColor;

void main()
{
  gl_FragColor =  vec4( textColor, 1.0 ) * texture2D( charImage, vec2( UV.x, 1.0 - UV.y ) );
}