#version 330 core
out vec4 FragColor;

const float gamma = 2.2;

in vec3 localPos;

uniform samplerCube environmentMap;
uniform float exposure;

void main() {
    vec3 envColor = texture(environmentMap, localPos).rgb;
    envColor = textureLod(environmentMap, localPos, 1.2).rgb;

    // tone mapping
    // ------------
    envColor = vec3(1.0) - exp(-envColor * exposure);
    envColor = pow(envColor, vec3(1.0 / gamma));

    FragColor = vec4(envColor, 1.0);
}