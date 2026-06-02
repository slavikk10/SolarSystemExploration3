// Credits to Sebastian Lague (github.com/SebLague) for a great atmosphere tutorial!
// 

#version 410 core
#define MAX_ARRAY_SIZE 8
out vec4 FragColor;

in GEOMETRY_OUT {
    vec2 TexCoords;
    vec3 viewVector;
    vec3 WorldPos;
} frag_in;

uniform uint numOfPlanets = 2;

uniform float densityFalloff;
uniform float scatteringStrength;

uniform vec3 camPos;
uniform vec3 lightPos;

uniform vec3 planetWorldPos[MAX_ARRAY_SIZE];
uniform float planetRadius[MAX_ARRAY_SIZE];

uniform float atmosphereHeight[MAX_ARRAY_SIZE];

uniform vec3 wavelengths[MAX_ARRAY_SIZE];

const int NUM_SAMPLES = 10;

uniform sampler2D depthTex;
uniform sampler2D colorTex;
uniform sampler2D opticalDepthTex;

vec2 raySphere(vec3 planetWorldPos, float atmosphereRadius, vec3 rayOrigin, vec3 rayDir) {
    vec3 offset = rayOrigin - planetWorldPos;
    float a = 1.0;
    float b = 2.0 * dot(offset, rayDir);
    float c = dot(offset, offset) - atmosphereRadius * atmosphereRadius;
    float d = b * b - 4 * a * c;

    if (d > 0.0) {
        float s = sqrt(d);
        float dstToSphereNear = max(0.0, (-b - s) / (2 * a));
        float dstToSphereFar = (-b + s) / (2.0 * a);

        if (dstToSphereFar >= 0.0) {
            return vec2(dstToSphereNear, dstToSphereFar - dstToSphereNear);
        }
    }

    return vec2(100000000000000000.0, 0.0);
}

float density(vec3 samplePoint, uint i, float atmosphereRadius) {
    float height = length(samplePoint - planetWorldPos[i]) - planetRadius[i];
    return exp(-(height / (atmosphereRadius - planetRadius[i])) * densityFalloff) * (1 - (height / (atmosphereRadius - planetRadius[i])));
}

float opticalDepth(vec3 rayOrigin, vec3 rayDir, float rayLength, uint id, float atmosphereRadius) {
    vec3 densitySamplePoint = rayOrigin;
    float stepSize = rayLength / (NUM_SAMPLES - 1);
    float opticalDepth = 0;

    for (uint i = 0; i < NUM_SAMPLES; i++) {
        float density = density(densitySamplePoint, id, atmosphereRadius);
        opticalDepth += density * stepSize;
        densitySamplePoint += rayDir * stepSize;
    }
    return opticalDepth;
}

float opticalDepthBaked(vec3 rayOrigin, vec3 rayDir, uint i, float atmosphereRadius) {
    float height   = length(rayOrigin - planetWorldPos[i]) - planetRadius[i];
    float height01 = clamp(height / (atmosphereRadius - planetRadius[i]), 0.0, 1.0);

    float texCoordY = dot(normalize(rayOrigin - planetWorldPos[i]), -rayDir) * 0.5 + 0.5;
    return clamp(texture(opticalDepthTex, vec2(height01, texCoordY)).r, 0.0, 100000000000.0);
}

float opticalDepthBaked2(vec3 rayOrigin, vec3 rayDir, float rayLength, uint i, float atmosphereRadius) {
	vec3 endPoint = rayOrigin + rayDir * rayLength;
	float d = dot(rayDir, normalize(rayOrigin - planetWorldPos[i]));
	float opticalDepth = 0.0;

	const float blendStrength = 1.5;
	float w = clamp(d * blendStrength + 0.5, 0.0, 1.0);
				
	float d1 = opticalDepthBaked(rayOrigin, rayDir, i, atmosphereRadius) - opticalDepthBaked(endPoint,   rayDir, i, atmosphereRadius);
	float d2 = opticalDepthBaked(endPoint, -rayDir, i, atmosphereRadius) - opticalDepthBaked(rayOrigin, -rayDir, i, atmosphereRadius);

	opticalDepth = mix(d2, d1, w);
	return opticalDepth;
}

vec3 calculateLight(vec3 rayOrigin, vec3 rayDir, float rayLength, vec3 originalCol, vec3 scatteringCoefficients, uint id, float atmosphereRadius) {
    vec3 inScatterPoint = rayOrigin;
    float stepSize = rayLength / (NUM_SAMPLES - 1);
    vec3 inScatteredLight = vec3(0.0);
    float viewRayOpticalDepth = 0.0;

    for (uint i = 0; i < NUM_SAMPLES; i++) {
        vec3 dirToLight = normalize(lightPos - inScatterPoint);
        float sunRayLength = raySphere(planetWorldPos[id], atmosphereRadius, inScatterPoint, dirToLight).y;
        float sunRayOpticalDepth = opticalDepthBaked(inScatterPoint + dirToLight * 0.8, dirToLight, id, atmosphereRadius);
        viewRayOpticalDepth = opticalDepthBaked2(rayOrigin, rayDir, stepSize * i, id, atmosphereRadius);
        vec3 transmittance = exp(-(sunRayOpticalDepth + viewRayOpticalDepth) * scatteringCoefficients);
        float density = density(inScatterPoint, id, atmosphereRadius);

        inScatteredLight += density * transmittance * scatteringCoefficients * stepSize;
        inScatterPoint += rayDir * stepSize;
    }

    vec3 finalColor = originalCol * exp(-viewRayOpticalDepth * scatteringCoefficients) + inScatteredLight;
    return finalColor;
}

void main() {
    vec4 originalCol = texture(colorTex, frag_in.TexCoords);
    float linearDepth = pow(2.0, texture(depthTex, frag_in.TexCoords).r * 49.829) - 1.0;
    
    vec3 viewDir = normalize(frag_in.viewVector);

    vec4 color = vec4(0.0);
    for (uint i = 0; i < numOfPlanets; i++) {
        float atmosphereRadius = planetRadius[i] + atmosphereHeight[i];

        float scatterR = pow(1 / wavelengths[i].x, 4) * scatteringStrength;
        float scatterG = pow(1 / wavelengths[i].y, 4) * scatteringStrength;
        float scatterB = pow(1 / wavelengths[i].z, 4) * scatteringStrength;
        vec3 scatteringCoefficients = vec3(scatterR, scatterG, scatterB);

        float surfaceHitInfo = raySphere(planetWorldPos[i], planetRadius[i], vec3(0.0), viewDir).x;
        float dstToSurface = min(linearDepth, surfaceHitInfo);

        vec2 hitInfo = raySphere(planetWorldPos[i], atmosphereRadius, vec3(0.0), viewDir);
        float dstToAtmosphere = hitInfo.x;
        float dstThroughAtmosphere = min(hitInfo.y, dstToSurface - dstToAtmosphere);

        if (dstThroughAtmosphere > 0) {
            vec3 pointInAtmosphere = vec3(0.0) + viewDir * dstToAtmosphere;
            vec3 light = calculateLight(pointInAtmosphere, viewDir, dstThroughAtmosphere, originalCol.rgb / numOfPlanets, scatteringCoefficients, i, atmosphereRadius);
            color += vec4(light, 1.0);
        } else {
            color += originalCol / numOfPlanets;
        }
    }

    FragColor = color;
}