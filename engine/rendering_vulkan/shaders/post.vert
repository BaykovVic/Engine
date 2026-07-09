#version 450
// Fullscreen triangle from gl_VertexIndex — no vertex buffer. The three
// vertices land at (-1,-1), (3,-1), (-1,3) so the clipped triangle covers
// the viewport exactly once; uv maps 1:1 onto the HDR scene target.
layout(location = 0) out vec2 vUv;

void main() {
    vec2 pos = vec2((gl_VertexIndex << 1) & 2, gl_VertexIndex & 2);
    vUv = pos;
    gl_Position = vec4(pos * 2.0 - 1.0, 0.0, 1.0);
}
