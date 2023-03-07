#version 110
//#extension GL_ARB_draw_instanced : enable
//#define gl_InstanceID gl_InstanceIDEXT
#extension GL_EXT_gpu_shader4 : enable

attribute vec4 vertex; // <vec2 position, vec2 texCoords>

varying vec2 TexCoords;
varying vec4 ParticleColor;

uniform mat4 projection;
uniform vec2 offset[500];
uniform vec4 color[500];

void main()
{
    float scale = 10.0;
    TexCoords = vertex.zw;
    ParticleColor = color[gl_InstanceID];
    gl_Position = projection * vec4((vertex.xy * scale) + offset[gl_InstanceID], 0.0, 1.0);
}
