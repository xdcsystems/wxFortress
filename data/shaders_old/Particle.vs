#version 330 core
layout (location = 0) in vec4 vertex; // <vec2 position, vec2 texCoords>

out vec2 TexCoords;
out vec4 ParticleColor;

uniform mat4 projection;
uniform vec2 offset[500];
uniform vec4 color[500];

void main()
{
    float scale = 10.0f;
    TexCoords = vertex.zw;
    ParticleColor = color[gl_InstanceID];
    gl_Position = projection * vec4((vertex.xy * scale) + offset[gl_InstanceID], 0.0, 1.0);
}