#version 410 core
out vec4 FragColor;

uniform vec3 color;
uniform vec3 WorldPos;

void main()
{
    gl_FragDepth = log2(length(WorldPos) + 1.0) / log2(1000000000000001.0);
    FragColor = vec4(color, 1.0);
}