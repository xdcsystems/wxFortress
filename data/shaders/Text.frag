#version 120

// Interpolated values from the vertex shaders
varying vec2 UV;

// Values that stay constant for the whole mesh.
uniform sampler2D charImage;

void main()
{
  gl_FragColor = texture2D( charImage, vec2( UV.x, 1.0 - UV.y ) );
}