#version 110

attribute vec4 vertex; // <vec2 position, vec2 texCoords>

varying vec2 TexCoords;

uniform vec4 rect;   // <pos.x, pos.y, size.x, size.y>
uniform vec4 uvRect; // <uOffset, vOffset, uScale, vScale> in top-down image space
uniform mat4 projection;

void main()
{
    vec2 uv = uvRect.xy + vertex.zw * uvRect.zw;
    TexCoords = vec2( uv.x, 1.0 - uv.y );
    gl_Position = projection * vec4( vertex.xy * rect.zw + rect.xy, 0.0, 1.0 );
}
