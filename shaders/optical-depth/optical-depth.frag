#version 410 core
out float FragColor;

in vec2 TexCoords;

uint NUM_SAMPLES       = 10;
float planetRadius     = 6371000.0;
float atmosphereRadius = 1000000.0 + planetRadius;
float densityFalloff   = 12.43;

float density(vec2 samplePoint) {
    float height   = length(samplePoint) - planetRadius;
    float height01 = height / (atmosphereRadius - planetRadius);
    return exp(-height01 * densityFalloff) * (1 - height01);
}

float opticalDepth(vec2 rayOrigin, vec2 rayDir, float rayLength) {
	vec2 densitySamplePoint = rayOrigin;
	float stepSize = rayLength / (NUM_SAMPLES - 1);
	float opticalDepth = 0;

	for (int i = 0; i < NUM_SAMPLES; i++) {
		float density = density(densitySamplePoint);
		opticalDepth += density * stepSize;
		densitySamplePoint += rayDir * stepSize;
	}
	return opticalDepth;
}

vec2 raySphere(float atmosphereRadius, vec3 rayOrigin, vec3 rayDir) {
    float a = 1;
    float b = 2 * dot(rayOrigin, rayDir);
    float c = dot(rayOrigin, rayOrigin) - atmosphereRadius * atmosphereRadius;
    float d = b * b - 4 * a * c;

    if (d > 0) {
        float s = sqrt(d);
        float dstToSphereNear = max(0, (-b - s) / (2 * a));
        float dstToSphereFar = (-b + s) / (2 * a);

        if (dstToSphereFar >= 0) {
            return vec2(dstToSphereNear, dstToSphereFar - dstToSphereNear);
        }
    }

    return vec2(100000000000000000.0, 0.0);
}

void main()
{
    vec2 ndcFrag = TexCoords;

    float height = ndcFrag.x;

    float angle  = ndcFrag.y * 3.141592653589793;
    vec2 dir = vec2(sin(angle), cos(angle));

    float y = -2.0 * ndcFrag.y + 1.0;
	float x = sin(acos(y));
    dir = vec2(x, y);

    vec2 inPoint = vec2(0.0, mix(planetRadius, atmosphereRadius, height));
    float dstThroughAtmosphere = raySphere(atmosphereRadius, vec3(inPoint, 0.0), vec3(dir, 0.0)).y;
    vec2 outPoint = inPoint + dir * dstThroughAtmosphere;

    FragColor = opticalDepth(inPoint + dir * 0.0001, dir, dstThroughAtmosphere - 0.0002);
}