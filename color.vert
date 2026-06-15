#version 450

#extension GL_EXT_buffer_reference : require

layout(location = 0) out vec3 outColor;
layout(location = 1) out vec3 outNormal;
layout(location = 2) out vec3 outSunDirection;
layout(location = 3) out vec2 outUV;

struct Vertex {
    vec3 position;
    float uv_x;
    vec3 normal;
    float uv_y;
    vec4 color;
};

layout(buffer_reference, std430) readonly buffer VertexBuffer {
    Vertex vertices[];
};

layout(push_constant) uniform gpu_scene_data {
    mat4 viewProj;
    mat4 invViewProj;
    mat4 model;
    vec4 sunDirection;
    VertexBuffer ssbo;
    uint ssboOffset;
}
scene;

void main() {
    Vertex v = scene.ssbo.vertices[gl_VertexIndex + scene.ssboOffset];

    gl_Position = scene.viewProj * scene.model * vec4(v.position, 1.0);

    outColor = v.color.xyz;
    outNormal = mat3(scene.model) * v.normal;
    outSunDirection = scene.sunDirection.xyz;
    outUV = vec2(v.uv_x, v.uv_y);
}
