#version 410 core
layout (location = 0) in vec2 aPos;
layout (location = 1) in vec2 aTexCoords;

out VERTEX_OUT {
    vec2 TexCoords;
    vec3 viewVector;
    vec3 WorldPos;
} vert_out;

uniform mat4 invViewProj;

void main() {
    gl_Position = vec4(aPos, 0.0, 1.0);
    vert_out.TexCoords = aTexCoords;

    vec4 clipPos = vec4(aTexCoords * 2.0 - 1.0, 0.0, 1.0);
    vec4 worldPos4 = invViewProj * clipPos;

    vert_out.viewVector = vec3(worldPos4);
    vert_out.WorldPos = worldPos4.xyz / worldPos4.w;
}