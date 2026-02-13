#version 410 core
out vec4 FragColor;

in GEOMETRY_OUT {
    vec2 TexCoords;
    vec3 viewVector;
    vec3 WorldPos;
} frag_in;

const float densityFalloff = 12.43;
const float scatteringStrength = 250000.0;

uniform vec3 camPos;
uniform vec3 lightPos;

uniform vec3 planetWorldPos;
uniform float planetRadius;

uniform float atmosphereHeight;

uniform vec3 wavelengths;

float atmosphereRadius = planetRadius + atmosphereHeight;

float scatterR = pow(1 / wavelengths.x, 4) * scatteringStrength;
float scatterG = pow(1 / wavelengths.y, 4) * scatteringStrength;
float scatterB = pow(1 / wavelengths.z, 4) * scatteringStrength;
vec3 scatteringCoefficients = vec3(scatterR, scatterG, scatterB);

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

float density(vec3 samplePoint) {
    float height = length(samplePoint - planetWorldPos) - planetRadius;
    return exp(-(height / (atmosphereRadius - planetRadius)) * densityFalloff) * (1 - (height / (atmosphereRadius - planetRadius)));
}

float opticalDepth(vec3 rayOrigin, vec3 rayDir, float rayLength) {
    vec3 densitySamplePoint = rayOrigin;
    float stepSize = rayLength / (NUM_SAMPLES - 1);
    float opticalDepth = 0;

    for (int i = 0; i < NUM_SAMPLES; i++) {
        float density = density(densitySamplePoint);
        opticalDepth += density * stepSize;
        densitySamplePoint += rayDir * stepSize;
    }
    return opticalDepth;
}

float opticalDepthBaked(vec3 rayOrigin, vec3 rayDir) {
    float height   = length(rayOrigin - planetWorldPos) - planetRadius;
    float height01 = clamp(height / (atmosphereRadius - planetRadius), 0.0, 1.0);

    float texCoordY = dot(normalize(rayOrigin - planetWorldPos), -rayDir) * 0.5 + 0.5;
    return clamp(texture(opticalDepthTex, vec2(height01, texCoordY)).r, 0.0, 100000000000.0);
}

float opticalDepthBaked2(vec3 rayOrigin, vec3 rayDir, float rayLength) {
	vec3 endPoint = rayOrigin + rayDir * rayLength;
	float d = dot(rayDir, normalize(rayOrigin - planetWorldPos));
	float opticalDepth = 0.0;

	const float blendStrength = 1.5;
	float w = clamp(d * blendStrength + 0.5, 0.0, 1.0);
				
	float d1 = opticalDepthBaked(rayOrigin, rayDir) - opticalDepthBaked(endPoint, rayDir);
	float d2 = opticalDepthBaked(endPoint, -rayDir) - opticalDepthBaked(rayOrigin, -rayDir);

	opticalDepth = mix(d2, d1, w);
	return opticalDepth;
}

vec3 calculateLight(vec3 rayOrigin, vec3 rayDir, float rayLength, vec3 originalCol) {
    vec3 inScatterPoint = rayOrigin;
    float stepSize = rayLength / (NUM_SAMPLES - 1);
    vec3 inScatteredLight = vec3(0.0);
    float viewRayOpticalDepth = 0.0;

    for (int i = 0; i < NUM_SAMPLES; i++) {
        vec3 dirToLight = normalize(lightPos - inScatterPoint);
        float sunRayLength = raySphere(planetWorldPos, atmosphereRadius, inScatterPoint, dirToLight).y;
        float sunRayOpticalDepth = opticalDepthBaked(inScatterPoint + dirToLight * 0.8, dirToLight);
        viewRayOpticalDepth = opticalDepthBaked2(rayOrigin, rayDir, stepSize * i);
        vec3 transmittance = exp(-(sunRayOpticalDepth + viewRayOpticalDepth) * scatteringCoefficients);
        float density = density(inScatterPoint);

        inScatteredLight += density * transmittance * scatteringCoefficients * stepSize;
        inScatterPoint += rayDir * stepSize;
    }
    /*inScatteredLight *= scatteringCoefficients * 1 * stepSize / planetRadius;

    const float brightnessAdaptionStrength = 0.15;
    const float reflectedLightOutScatterStrength = 3;
    float brightnessAdaption = dot(inScatteredLight, vec3(1.0)) * brightnessAdaptionStrength;
    float brightnessSum = viewRayOpticalDepth * 1 * reflectedLightOutScatterStrength + brightnessAdaption;
    float reflectedLightStrength = exp(-brightnessSum);
    float hdrStrength = clamp(dot(originalCol, vec3(1.0)) / 3 - 1, 0.0, 1.0);
    reflectedLightStrength = mix(reflectedLightStrength, 1, hdrStrength);
    vec3 reflectedLight = originalCol * reflectedLightStrength;*/

    vec3 finalColor = originalCol * exp(-viewRayOpticalDepth * scatteringCoefficients) + inScatteredLight;
    return finalColor;
}

void main() {
    vec4 originalCol = texture(colorTex, frag_in.TexCoords);
    float linearDepth = pow(2.0, texture(depthTex, frag_in.TexCoords).r * 49.829) - 1.0;
    
    vec3 viewDir = normalize(frag_in.viewVector);

    float surfaceHitInfo = raySphere(planetWorldPos, planetRadius, vec3(0.0), viewDir).x;
    float dstToSurface = min(linearDepth, surfaceHitInfo);

    vec2 hitInfo = raySphere(planetWorldPos, atmosphereRadius, vec3(0.0), viewDir);
    float dstToAtmosphere = hitInfo.x;
    float dstThroughAtmosphere = min(hitInfo.y, dstToSurface - dstToAtmosphere);

    if (dstThroughAtmosphere > 0) {
        vec3 pointInAtmosphere = vec3(0.0) + viewDir * dstToAtmosphere;
        vec3 light = calculateLight(pointInAtmosphere, viewDir, dstThroughAtmosphere, originalCol.rgb);
        FragColor = vec4(light, 1.0);
    } else {
        FragColor = originalCol;
    }
}