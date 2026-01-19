#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec3 inColor;

layout(location = 0) out vec3 outColor;
layout(location = 1) out vec3 outNormal;
layout(location = 2) out vec3 outSunDirection;

layout(push_constant) uniform gpu_scene_data {
    mat4 viewProj;
    mat4 invViewProj;
    mat4 model;
    vec3 sunDirection;
}
scene;

void main() {
    gl_Position = scene.viewProj * scene.model * vec4(inPosition, 1.0);
    outColor = inColor;
    outNormal = mat3(scene.model) * inNormal;
    outSunDirection = scene.sunDirection;
}
