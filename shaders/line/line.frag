#version 410 core
out vec4 FragColor;

uniform vec3 color;

void main()
{
    gl_FragDepth = 0.0;
    FragColor = vec4(color, 1.0);
}