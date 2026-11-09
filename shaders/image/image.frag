#version 410 core
in vec2 TexCoords;
out vec4 color;

uniform sampler2D image;
uniform float transparency;
uniform float brightness;

void main()
{
    vec3 rgb = texture(image, TexCoords).rgb * brightness;
    color = vec4(rgb, texture(image, TexCoords).a * transparency);
    gl_FragDepth = 0.0; // to make the image be always in front of everything else
}