#version 110

attribute vec4 vertex; // <vec2 position, vec2 texCoords>

varying vec2 TexCoords;

uniform vec4 rect; // <pos.x, pos.y, size.x, size.y>
uniform mat4 projection;

void main()
{
    TexCoords = vec2( vertex.z, 1.0 - vertex.w );
    gl_Position = projection * vec4( vertex.xy * rect.zw + rect.xy, 0.0, 1.0 );
}