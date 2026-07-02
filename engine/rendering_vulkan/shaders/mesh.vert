#version 450
// Engine vertex format: position(3) + normal(3) + uv(2).
layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aUv;

layout(set = 0, binding = 0) uniform Frame {
    mat4 viewProjection;
    vec4 cameraPos;     // xyz used
    vec4 lightVec[4];   // direction (directional) or position (point)
    vec4 lightColor[4]; // rgb premultiplied by intensity
    vec4 lightMeta[4];  // x = type (0 dir, 1 point), y = range
    vec4 counts;        // x = light count, y = shadows on, z = shadow light
    mat4 lightViewProjection;
} frame;

layout(push_constant) uniform Push {
    mat4 model;
    vec4 baseColor; // w = skyMode
    vec4 emissive;  // w = roughness
    vec4 params;    // x = metallic
    vec4 params2;   // xy = uvTiling, z = parallaxDepth
} pc;

layout(location = 0) out vec3 vWorldPos;
layout(location = 1) out vec3 vNormal;
layout(location = 2) out vec2 vUv;

void main() {
    vec4 world = pc.model * vec4(aPosition, 1.0);
    vWorldPos = world.xyz;
    vNormal = mat3(pc.model) * aNormal;
    vUv = aUv;
    gl_Position = frame.viewProjection * world;
}
