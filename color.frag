#version 450

layout(location = 0) in vec3 inColor;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec3 inSunDirection;

layout(location = 0) out vec4 outColor;

void main() {
    vec3 lightDir = normalize(inSunDirection);
    vec3 n = normalize(inNormal);

    float sunHeight = lightDir.y;

    float dayFactor = smoothstep(-0.1, 0.1, sunHeight);

    float diffuse = max(0.0, dot(n, lightDir));

    float nightAmbient = 0.02;
    float dayAmbient = 0.1;

    float finalAmbient = mix(nightAmbient, dayAmbient, dayFactor);
    float finalDiffuse = diffuse * dayFactor;

    vec3 col = (finalAmbient + finalDiffuse) * inColor;

    outColor = vec4(col, 1.0);
}
