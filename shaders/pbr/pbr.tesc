#version 410 core
layout (vertices=3) out;

in VERTEX_OUT {
    vec2 TextureCoords;
    vec3 WorldPos;
    vec3 Normal;
} vert_in[];

out vec2 TextureCoorde[];
out vec3 WorldPose[];
out vec3 Normale[];

void main() {
    gl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;
    TextureCoorde[gl_InvocationID]      = vert_in[gl_InvocationID].TextureCoords;
    WorldPose[gl_InvocationID]          = vert_in[gl_InvocationID].WorldPos;
    Normale[gl_InvocationID]            = vert_in[gl_InvocationID].Normal;

    if (gl_InvocationID == 0) {
        float tessLevel = 16.0;

        gl_TessLevelOuter[0] = tessLevel;
        gl_TessLevelOuter[1] = tessLevel;
        gl_TessLevelOuter[2] = tessLevel;

        gl_TessLevelInner[0] = tessLevel;
    }
}