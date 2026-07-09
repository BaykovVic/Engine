#version 450
// Post pass: reads the HDR scene target and writes the presented (or
// readback) image. exposure <= 0 is a bit-exact passthrough so frames
// rendered without tonemapping keep their reference colours; exposure > 0
// applies the ACES filmic curve after exposure scaling.
layout(location = 0) in vec2 vUv;

layout(set = 0, binding = 0) uniform sampler2D uHdr;

layout(push_constant) uniform Post {
    vec4 params; // x = exposure (0 = passthrough)
} pc;

layout(location = 0) out vec4 fragColor;

// Narkowicz ACES approximation.
vec3 acesTonemap(vec3 x) {
    const float a = 2.51;
    const float b = 0.03;
    const float c = 2.43;
    const float d = 0.59;
    const float e = 0.14;
    return clamp((x * (a * x + b)) / (x * (c * x + d) + e), 0.0, 1.0);
}

void main() {
    vec4 hdr = texture(uHdr, vUv);
    if (pc.params.x <= 0.0) {
        fragColor = hdr;
        return;
    }
    fragColor = vec4(acesTonemap(hdr.rgb * pc.params.x), hdr.a);
}
