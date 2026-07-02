#version 450
// Depth-only shadow pass: rasterize every mesh from the light's point of
// view. No descriptor sets — both matrices ride in the push constants
// (128 bytes, the guaranteed minimum).
layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec3 aNormal; // present in the vertex format, unused
layout(location = 2) in vec2 aUv;     // present in the vertex format, unused

layout(push_constant) uniform Push {
    mat4 lightViewProjection;
    mat4 model;
} pc;

void main() {
    gl_Position = pc.lightViewProjection * pc.model * vec4(aPosition, 1.0);
}
