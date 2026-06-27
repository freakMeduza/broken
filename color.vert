#version 450

#extension GL_EXT_buffer_reference : require

layout(location = 0) out vec3 outColor;
layout(location = 1) out vec3 outNormal;
layout(location = 2) out vec2 outUV;

layout(set = 0, binding = 1) uniform scene_uniform_buffer {
    mat4 viewMatrix;
    mat4 projMatrix;
    vec4 sunDirection;
}
scene;

struct Vertex {
    vec3 position;
    float uv_x;
    vec3 normal;
    float uv_y;
    vec4 color;
};

layout(buffer_reference) readonly buffer VertexBuffer {
    Vertex vertices[];
};

layout(buffer_reference) readonly buffer IndexBuffer {
    uint indices[];
};

layout(push_constant) uniform scene_constants {
    mat4 modelMatrix;
    VertexBuffer vertexBuffer;
    uint vertexBufferOffset;
    IndexBuffer indexBuffer;
    uint indexBufferOffset;
}
push;

void main() {
    uint indexBufferIndex = gl_VertexIndex + push.indexBufferOffset;
    uint vertexIndex = push.indexBuffer.indices[indexBufferIndex];
    uint vertexBufferIndex = vertexIndex + push.vertexBufferOffset;
    Vertex v = push.vertexBuffer.vertices[vertexBufferIndex];

    gl_Position = scene.projMatrix * scene.viewMatrix * push.modelMatrix * vec4(v.position, 1.0);

    outColor = v.color.xyz;
    outNormal = mat3(push.modelMatrix) * v.normal;
    outUV = vec2(v.uv_x, v.uv_y);
}
