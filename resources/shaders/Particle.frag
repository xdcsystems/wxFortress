#version 110

varying vec2 TexCoords;
varying vec4 ParticleColor;

uniform sampler2D sprite;

void main()
{
    // GPU-first plasma tail gradient: cool cyan outer fade -> hot white-amber core by ParticleColor.a,
    // preserving >1.0 RGB intensity multiplier on soft sprite edges before RGBA8 clamp.
    vec3 tailTint = mix( vec3( 0.45, 0.85, 1.0 ), vec3( 1.0, 0.96, 0.88 ), clamp( ParticleColor.a, 0.0, 1.0 ) );
    vec4 tinted = vec4( ParticleColor.rgb * tailTint, ParticleColor.a );
    gl_FragColor = texture2D( sprite, TexCoords ) * tinted;
}
