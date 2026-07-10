// Self-contained OpenGL 3.3 core backend: GL types, constants and the
// handful of entry points it needs are declared here and resolved through
// the host-provided loader, so the module has no link-time GL dependency.

#include <array>
#include <cmath>
#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

#include "sky/rendering_opengl/opengl_backend.hpp"

namespace sky::rendering_opengl {
namespace {

// --- Minimal GL surface ----------------------------------------------------

using GLenum = std::uint32_t;
using GLuint = std::uint32_t;
using GLint = std::int32_t;
using GLsizei = std::int32_t;
using GLsizeiptr = std::intptr_t;
using GLfloat = float;
using GLboolean = std::uint8_t;
using GLchar = char;
using GLbitfield = std::uint32_t;

constexpr GLenum GL_DEPTH_TEST = 0x0B71;
constexpr GLenum GL_CULL_FACE = 0x0B44;
constexpr GLenum GL_COLOR_BUFFER_BIT = 0x4000;
constexpr GLenum GL_DEPTH_BUFFER_BIT = 0x0100;
constexpr GLenum GL_ARRAY_BUFFER = 0x8892;
constexpr GLenum GL_STATIC_DRAW = 0x88E4;
constexpr GLenum GL_TRIANGLES = 0x0004;
constexpr GLenum GL_FLOAT = 0x1406;
constexpr GLenum GL_FRAGMENT_SHADER = 0x8B30;
constexpr GLenum GL_VERTEX_SHADER = 0x8B31;
constexpr GLenum GL_COMPILE_STATUS = 0x8B81;
constexpr GLenum GL_LINK_STATUS = 0x8B82;
constexpr GLenum GL_LINE = 0x1B01;
constexpr GLenum GL_FILL = 0x1B02;
constexpr GLenum GL_FRONT_AND_BACK = 0x0408;
constexpr GLenum GL_TEXTURE_2D = 0x0DE1;
constexpr GLenum GL_RGBA = 0x1908;
constexpr GLenum GL_RGBA8 = 0x8058;
constexpr GLenum GL_UNSIGNED_BYTE = 0x1401;
constexpr GLenum GL_TEXTURE_MIN_FILTER = 0x2801;
constexpr GLenum GL_TEXTURE_MAG_FILTER = 0x2800;
constexpr GLenum GL_LINEAR = 0x2601;
constexpr GLenum GL_LINEAR_MIPMAP_LINEAR = 0x2703;
constexpr GLenum GL_TEXTURE_WRAP_S = 0x2802;
constexpr GLenum GL_TEXTURE_WRAP_T = 0x2803;
constexpr GLenum GL_REPEAT = 0x2901;
constexpr GLenum GL_TEXTURE0 = 0x84C0;
constexpr GLenum GL_LEQUAL = 0x0203;
constexpr GLenum GL_LESS = 0x0201;
constexpr GLenum GL_FRAMEBUFFER = 0x8D40;
constexpr GLenum GL_DEPTH_ATTACHMENT = 0x8D00;
constexpr GLenum GL_DEPTH_COMPONENT = 0x1902;
constexpr GLenum GL_DEPTH_COMPONENT24 = 0x81A6;
constexpr GLenum GL_FRAMEBUFFER_COMPLETE = 0x8CD5;
constexpr GLenum GL_FRAMEBUFFER_BINDING = 0x8CA6;
constexpr GLenum GL_TEXTURE1 = 0x84C1;
constexpr GLenum GL_CLAMP_TO_EDGE = 0x812F;
constexpr GLenum GL_NEAREST = 0x2600;
constexpr GLenum GL_NONE_MODE = 0;

struct GlApi {
    void (*Enable)(GLenum) = nullptr;
    void (*Disable)(GLenum) = nullptr;
    void (*ClearColor)(GLfloat, GLfloat, GLfloat, GLfloat) = nullptr;
    void (*Clear)(GLbitfield) = nullptr;
    void (*Viewport)(GLint, GLint, GLsizei, GLsizei) = nullptr;
    GLuint (*CreateShader)(GLenum) = nullptr;
    void (*ShaderSource)(GLuint, GLsizei, const GLchar* const*, const GLint*) = nullptr;
    void (*CompileShader)(GLuint) = nullptr;
    void (*GetShaderiv)(GLuint, GLenum, GLint*) = nullptr;
    GLuint (*CreateProgram)() = nullptr;
    void (*AttachShader)(GLuint, GLuint) = nullptr;
    void (*LinkProgram)(GLuint) = nullptr;
    void (*GetProgramiv)(GLuint, GLenum, GLint*) = nullptr;
    void (*DeleteShader)(GLuint) = nullptr;
    void (*UseProgram)(GLuint) = nullptr;
    GLint (*GetUniformLocation)(GLuint, const GLchar*) = nullptr;
    void (*UniformMatrix4fv)(GLint, GLsizei, GLboolean, const GLfloat*) = nullptr;
    void (*Uniform3f)(GLint, GLfloat, GLfloat, GLfloat) = nullptr;
    void (*Uniform2f)(GLint, GLfloat, GLfloat) = nullptr;
    void (*Uniform1f)(GLint, GLfloat) = nullptr;
    void (*Uniform1i)(GLint, GLint) = nullptr;
    void (*Uniform3fv)(GLint, GLsizei, const GLfloat*) = nullptr;
    void (*Uniform1iv)(GLint, GLsizei, const GLint*) = nullptr;
    void (*Uniform1fv)(GLint, GLsizei, const GLfloat*) = nullptr;
    void (*GenVertexArrays)(GLsizei, GLuint*) = nullptr;
    void (*BindVertexArray)(GLuint) = nullptr;
    void (*GenBuffers)(GLsizei, GLuint*) = nullptr;
    void (*BindBuffer)(GLenum, GLuint) = nullptr;
    void (*BufferData)(GLenum, GLsizeiptr, const void*, GLenum) = nullptr;
    void (*EnableVertexAttribArray)(GLuint) = nullptr;
    void (*VertexAttribPointer)(GLuint, GLint, GLenum, GLboolean, GLsizei,
                                const void*) = nullptr;
    void (*DrawArrays)(GLenum, GLint, GLsizei) = nullptr;
    void (*PolygonMode)(GLenum, GLenum) = nullptr;
    void (*GenTextures)(GLsizei, GLuint*) = nullptr;
    void (*BindTexture)(GLenum, GLuint) = nullptr;
    void (*TexImage2D)(GLenum, GLint, GLint, GLsizei, GLsizei, GLint, GLenum, GLenum,
                       const void*) = nullptr;
    void (*TexParameteri)(GLenum, GLenum, GLint) = nullptr;
    void (*GenerateMipmap)(GLenum) = nullptr;
    void (*ActiveTexture)(GLenum) = nullptr;
    void (*DeleteTextures)(GLsizei, const GLuint*) = nullptr;
    void (*DepthMask)(GLboolean) = nullptr;
    void (*DepthFunc)(GLenum) = nullptr;
    void (*GenFramebuffers)(GLsizei, GLuint*) = nullptr;
    void (*BindFramebuffer)(GLenum, GLuint) = nullptr;
    void (*FramebufferTexture2D)(GLenum, GLenum, GLenum, GLuint, GLint) = nullptr;
    GLenum (*CheckFramebufferStatus)(GLenum) = nullptr;
    void (*DrawBuffer)(GLenum) = nullptr;
    void (*ReadBuffer)(GLenum) = nullptr;
    void (*GetIntegerv)(GLenum, GLint*) = nullptr;

    bool load(const GlLoader& loader) {
        const auto resolve = [&](auto& slot, const char* name) {
            slot = reinterpret_cast<std::remove_reference_t<decltype(slot)>>(
                loader(name));
            return slot != nullptr;
        };
        return resolve(Enable, "glEnable") && resolve(Disable, "glDisable") &&
               resolve(ClearColor, "glClearColor") && resolve(Clear, "glClear") &&
               resolve(Viewport, "glViewport") &&
               resolve(CreateShader, "glCreateShader") &&
               resolve(ShaderSource, "glShaderSource") &&
               resolve(CompileShader, "glCompileShader") &&
               resolve(GetShaderiv, "glGetShaderiv") &&
               resolve(CreateProgram, "glCreateProgram") &&
               resolve(AttachShader, "glAttachShader") &&
               resolve(LinkProgram, "glLinkProgram") &&
               resolve(GetProgramiv, "glGetProgramiv") &&
               resolve(DeleteShader, "glDeleteShader") &&
               resolve(UseProgram, "glUseProgram") &&
               resolve(GetUniformLocation, "glGetUniformLocation") &&
               resolve(UniformMatrix4fv, "glUniformMatrix4fv") &&
               resolve(Uniform3f, "glUniform3f") &&
               resolve(Uniform2f, "glUniform2f") &&
               resolve(Uniform1f, "glUniform1f") &&
               resolve(Uniform1i, "glUniform1i") &&
               resolve(Uniform3fv, "glUniform3fv") &&
               resolve(Uniform1iv, "glUniform1iv") &&
               resolve(Uniform1fv, "glUniform1fv") &&
               resolve(GenVertexArrays, "glGenVertexArrays") &&
               resolve(BindVertexArray, "glBindVertexArray") &&
               resolve(GenBuffers, "glGenBuffers") &&
               resolve(BindBuffer, "glBindBuffer") &&
               resolve(BufferData, "glBufferData") &&
               resolve(EnableVertexAttribArray, "glEnableVertexAttribArray") &&
               resolve(VertexAttribPointer, "glVertexAttribPointer") &&
               resolve(DrawArrays, "glDrawArrays") &&
               resolve(PolygonMode, "glPolygonMode") &&
               resolve(GenTextures, "glGenTextures") &&
               resolve(BindTexture, "glBindTexture") &&
               resolve(TexImage2D, "glTexImage2D") &&
               resolve(TexParameteri, "glTexParameteri") &&
               resolve(GenerateMipmap, "glGenerateMipmap") &&
               resolve(ActiveTexture, "glActiveTexture") &&
               resolve(DeleteTextures, "glDeleteTextures") &&
               resolve(DepthMask, "glDepthMask") &&
               resolve(DepthFunc, "glDepthFunc") &&
               resolve(GenFramebuffers, "glGenFramebuffers") &&
               resolve(BindFramebuffer, "glBindFramebuffer") &&
               resolve(FramebufferTexture2D, "glFramebufferTexture2D") &&
               resolve(CheckFramebufferStatus, "glCheckFramebufferStatus") &&
               resolve(DrawBuffer, "glDrawBuffer") &&
               resolve(ReadBuffer, "glReadBuffer") &&
               resolve(GetIntegerv, "glGetIntegerv");
    }
};

// --- Matrix helpers ---------------------------------------------------------

/// Column-major 4x4, as OpenGL expects it.
struct Mat4 {
    std::array<float, 16> m{};

    static Mat4 identity() {
        Mat4 r;
        r.m[0] = r.m[5] = r.m[10] = r.m[15] = 1.0f;
        return r;
    }

    Mat4 operator*(const Mat4& other) const {
        Mat4 r;
        for (int col = 0; col < 4; ++col) {
            for (int row = 0; row < 4; ++row) {
                float sum = 0.0f;
                for (int k = 0; k < 4; ++k) {
                    sum += m[k * 4 + row] * other.m[col * 4 + k];
                }
                r.m[col * 4 + row] = sum;
            }
        }
        return r;
    }
};

Mat4 fromTransform(const core::Transform& t) {
    const auto& q = t.rotation;
    const float xx = q.x * q.x, yy = q.y * q.y, zz = q.z * q.z;
    const float xy = q.x * q.y, xz = q.x * q.z, yz = q.y * q.z;
    const float wx = q.w * q.x, wy = q.w * q.y, wz = q.w * q.z;

    Mat4 r = Mat4::identity();
    r.m[0] = (1 - 2 * (yy + zz)) * t.scale.x;
    r.m[1] = (2 * (xy + wz)) * t.scale.x;
    r.m[2] = (2 * (xz - wy)) * t.scale.x;
    r.m[4] = (2 * (xy - wz)) * t.scale.y;
    r.m[5] = (1 - 2 * (xx + zz)) * t.scale.y;
    r.m[6] = (2 * (yz + wx)) * t.scale.y;
    r.m[8] = (2 * (xz + wy)) * t.scale.z;
    r.m[9] = (2 * (yz - wx)) * t.scale.z;
    r.m[10] = (1 - 2 * (xx + yy)) * t.scale.z;
    r.m[12] = t.position.x;
    r.m[13] = t.position.y;
    r.m[14] = t.position.z;
    return r;
}

Mat4 perspective(float fovDegrees, float aspect, float zNear, float zFar) {
    const float f = 1.0f / std::tan(fovDegrees * 3.14159265f / 360.0f);
    Mat4 r;
    r.m[0] = f / aspect;
    r.m[5] = f;
    r.m[10] = (zFar + zNear) / (zNear - zFar);
    r.m[11] = -1.0f;
    r.m[14] = 2.0f * zFar * zNear / (zNear - zFar);
    return r;
}

Mat4 orthographic(float halfExtent, float zNear, float zFar) {
    Mat4 r;
    r.m[0] = 1.0f / halfExtent;
    r.m[5] = 1.0f / halfExtent;
    r.m[10] = -2.0f / (zFar - zNear);
    r.m[14] = -(zFar + zNear) / (zFar - zNear);
    r.m[15] = 1.0f;
    return r;
}

Mat4 lookAt(const core::Vec3& eye, const core::Vec3& target, const core::Vec3& up) {
    const auto normalize = [](core::Vec3 v) {
        const float length = std::sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
        return length > 1e-6f ? core::Vec3{v.x / length, v.y / length, v.z / length}
                              : core::Vec3{0.0f, 0.0f, 1.0f};
    };
    const auto cross = [](const core::Vec3& a, const core::Vec3& b) {
        return core::Vec3{a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z,
                          a.x * b.y - a.y * b.x};
    };
    const auto f = normalize({target.x - eye.x, target.y - eye.y, target.z - eye.z});
    const auto r = normalize(cross(f, up));
    const auto u = cross(r, f);

    Mat4 view = Mat4::identity();
    view.m[0] = r.x; view.m[4] = r.y; view.m[8] = r.z;
    view.m[1] = u.x; view.m[5] = u.y; view.m[9] = u.z;
    view.m[2] = -f.x; view.m[6] = -f.y; view.m[10] = -f.z;
    view.m[12] = -(r.x * eye.x + r.y * eye.y + r.z * eye.z);
    view.m[13] = -(u.x * eye.x + u.y * eye.y + u.z * eye.z);
    view.m[14] = f.x * eye.x + f.y * eye.y + f.z * eye.z;
    return view;
}

/// Inverse of a rigid camera pose (rotation + translation).
Mat4 viewFromCameraPose(const core::Transform& camera) {
    const core::Quat inv{-camera.rotation.x, -camera.rotation.y, -camera.rotation.z,
                         camera.rotation.w};
    core::Transform view;
    view.rotation = inv;
    view.position = core::rotate(inv, {-camera.position.x, -camera.position.y,
                                       -camera.position.z});
    return fromTransform(view);
}

// --- Geometry ---------------------------------------------------------------

// Unit cube centred at the origin: position(3) + normal(3) + uv(2),
// 36 vertices, each face mapped 0..1.
constexpr float kCubeVertices[] = {
    // -Z
    -0.5f,-0.5f,-0.5f, 0,0,-1, 0,0,  0.5f, 0.5f,-0.5f, 0,0,-1, 1,1,  0.5f,-0.5f,-0.5f, 0,0,-1, 1,0,
    -0.5f,-0.5f,-0.5f, 0,0,-1, 0,0, -0.5f, 0.5f,-0.5f, 0,0,-1, 0,1,  0.5f, 0.5f,-0.5f, 0,0,-1, 1,1,
    // +Z
    -0.5f,-0.5f, 0.5f, 0,0,1, 0,0,   0.5f,-0.5f, 0.5f, 0,0,1, 1,0,   0.5f, 0.5f, 0.5f, 0,0,1, 1,1,
    -0.5f,-0.5f, 0.5f, 0,0,1, 0,0,   0.5f, 0.5f, 0.5f, 0,0,1, 1,1,  -0.5f, 0.5f, 0.5f, 0,0,1, 0,1,
    // -X
    -0.5f,-0.5f,-0.5f, -1,0,0, 0,0, -0.5f,-0.5f, 0.5f, -1,0,0, 1,0, -0.5f, 0.5f, 0.5f, -1,0,0, 1,1,
    -0.5f,-0.5f,-0.5f, -1,0,0, 0,0, -0.5f, 0.5f, 0.5f, -1,0,0, 1,1, -0.5f, 0.5f,-0.5f, -1,0,0, 0,1,
    // +X
     0.5f,-0.5f,-0.5f, 1,0,0, 0,0,   0.5f, 0.5f, 0.5f, 1,0,0, 1,1,   0.5f,-0.5f, 0.5f, 1,0,0, 1,0,
     0.5f,-0.5f,-0.5f, 1,0,0, 0,0,   0.5f, 0.5f,-0.5f, 1,0,0, 0,1,   0.5f, 0.5f, 0.5f, 1,0,0, 1,1,
    // -Y
    -0.5f,-0.5f,-0.5f, 0,-1,0, 0,0,  0.5f,-0.5f,-0.5f, 0,-1,0, 1,0,  0.5f,-0.5f, 0.5f, 0,-1,0, 1,1,
    -0.5f,-0.5f,-0.5f, 0,-1,0, 0,0,  0.5f,-0.5f, 0.5f, 0,-1,0, 1,1, -0.5f,-0.5f, 0.5f, 0,-1,0, 0,1,
    // +Y
    -0.5f, 0.5f,-0.5f, 0,1,0, 0,0,   0.5f, 0.5f, 0.5f, 0,1,0, 1,1,   0.5f, 0.5f,-0.5f, 0,1,0, 1,0,
    -0.5f, 0.5f,-0.5f, 0,1,0, 0,0,  -0.5f, 0.5f, 0.5f, 0,1,0, 0,1,   0.5f, 0.5f, 0.5f, 0,1,0, 1,1,
};

// Depth-only pass rendering the scene from the directional light.
const char* kShadowVertexShader = R"glsl(
#version 330 core
layout(location = 0) in vec3 aPosition;
uniform mat4 uModel;
uniform mat4 uLightSpace;
void main() {
    gl_Position = uLightSpace * uModel * vec4(aPosition, 1.0);
}
)glsl";

const char* kShadowFragmentShader = R"glsl(
#version 330 core
void main() {}
)glsl";

const char* kVertexShader = R"glsl(
#version 330 core
layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aUv;
uniform mat4 uModel;
uniform mat4 uViewProjection;
out vec3 vNormal;
out vec3 vWorldPos;
out vec2 vUv;
void main() {
    vec4 world = uModel * vec4(aPosition, 1.0);
    vWorldPos = world.xyz;
    vNormal = mat3(uModel) * aNormal;
    vUv = aUv;
    gl_Position = uViewProjection * world;
}
)glsl";

// Cook-Torrance metal-rough PBR with up to 4 scene lights, a sky backdrop
// mode, tangent-space normal/parallax mapping (tangent basis reconstructed
// from screen-space derivatives, so the engine vertex format stays
// position+normal+uv) and ambient occlusion.
const char* kFragmentShader = R"glsl(
#version 330 core
in vec3 vNormal;
in vec3 vWorldPos;
in vec2 vUv;
uniform vec3 uBaseColor;
uniform sampler2D uTexture;       // albedo       (unit 0)
uniform sampler2D uShadowMap;     // shadow depth (unit 1)
uniform sampler2D uNormalMap;     // tangent normal (unit 2)
uniform sampler2D uRoughnessMap;  // roughness    (unit 3)
uniform sampler2D uMetallicMap;   // metallic     (unit 4)
uniform sampler2D uOcclusionMap;  // ambient occ  (unit 5)
uniform sampler2D uHeightMap;     // parallax     (unit 6)
uniform vec2 uUvTiling;
uniform float uParallaxDepth;
uniform mat4 uLightSpace;
uniform int uHasShadow; // the first directional light casts shadows
uniform vec3 uEmissive;
uniform float uRoughness;
uniform float uMetallic;
uniform vec3 uCameraPos;
uniform int uSkyMode; // 1 = sky backdrop pass
uniform int uLightCount;
uniform vec3 uLightVec[4];   // direction (directional) or position (point)
uniform vec3 uLightColor[4]; // colour premultiplied by intensity
uniform int uLightType[4];   // 0 directional, 1 point
uniform float uLightRange[4];
out vec4 fragColor;

const float PI = 3.14159265359;

float distributionGGX(float ndh, float rough) {
    float a = rough * rough;
    float a2 = a * a;
    float d = ndh * ndh * (a2 - 1.0) + 1.0;
    return a2 / max(PI * d * d, 1e-7);
}
float geometrySchlick(float ndv, float rough) {
    float r = rough + 1.0;
    float k = (r * r) / 8.0;
    return ndv / (ndv * (1.0 - k) + k);
}
vec3 fresnelSchlick(float vdh, vec3 f0) {
    return f0 + (1.0 - f0) * pow(clamp(1.0 - vdh, 0.0, 1.0), 5.0);
}

void main() {
    if (uSkyMode == 1) {
        // Sky backdrop: uBaseColor = horizon, uEmissive = zenith; the sun
        // disc comes from the first directional light.
        vec3 dir = normalize(vWorldPos - uCameraPos);
        float t = clamp(dir.y * 1.6 + 0.18, 0.0, 1.0);
        vec3 skyColor = mix(uBaseColor, uEmissive, t);
        for (int i = 0; i < uLightCount; ++i) {
            if (uLightType[i] == 0) {
                float towardSun = max(dot(dir, -normalize(uLightVec[i])), 0.0);
                skyColor += uLightColor[i] * (pow(towardSun, 600.0) * 1.4 +
                                              pow(towardSun, 10.0) * 0.10);
                break;
            }
        }
        fragColor = vec4(skyColor, 1.0);
        return;
    }

    vec3 ng = normalize(vNormal);
    vec3 v = normalize(uCameraPos - vWorldPos);
    vec2 uv = vUv * uUvTiling;

    // Tangent basis from screen-space derivatives — no per-vertex tangents.
    vec3 dp1 = dFdx(vWorldPos);
    vec3 dp2 = dFdy(vWorldPos);
    vec2 duv1 = dFdx(uv);
    vec2 duv2 = dFdy(uv);
    vec3 dp2perp = cross(dp2, ng);
    vec3 dp1perp = cross(ng, dp1);
    vec3 tang = dp2perp * duv1.x + dp1perp * duv2.x;
    vec3 bitan = dp2perp * duv1.y + dp1perp * duv2.y;
    float invmax = inversesqrt(max(dot(tang, tang), dot(bitan, bitan)));
    mat3 tbn = mat3(tang * invmax, bitan * invmax, ng);

    // Parallax offset along the tangent-space view direction.
    if (uParallaxDepth > 0.0) {
        vec3 vTan = normalize(v * tbn); // transpose(tbn) * v
        float height = texture(uHeightMap, uv).r;
        uv += (vTan.xy / max(vTan.z, 0.3)) * (height - 0.5) * uParallaxDepth;
    }

    vec3 albedo = uBaseColor * texture(uTexture, uv).rgb;
    vec3 mapN = texture(uNormalMap, uv).xyz * 2.0 - 1.0;
    vec3 n = normalize(tbn * mapN);
    float rough = clamp(uRoughness * texture(uRoughnessMap, uv).r, 0.045, 1.0);
    float metal = clamp(uMetallic * texture(uMetallicMap, uv).r, 0.0, 1.0);
    float ao = texture(uOcclusionMap, uv).r;

    // Shadow factor from the first directional light's depth map: 3x3 PCF
    // with a slope-independent bias.
    float shadowFactor = 1.0;
    if (uHasShadow == 1) {
        vec4 lightSpace = uLightSpace * vec4(vWorldPos, 1.0);
        vec3 projected = lightSpace.xyz / lightSpace.w * 0.5 + 0.5;
        if (projected.x > 0.0 && projected.x < 1.0 && projected.y > 0.0 &&
            projected.y < 1.0 && projected.z < 1.0) {
            float bias = 0.0028;
            float lit = 0.0;
            vec2 texel = 1.0 / vec2(textureSize(uShadowMap, 0));
            for (int sx = -1; sx <= 1; ++sx) {
                for (int sy = -1; sy <= 1; ++sy) {
                    float depth = texture(uShadowMap,
                                          projected.xy + vec2(sx, sy) * texel).r;
                    lit += projected.z - bias > depth ? 0.0 : 1.0;
                }
            }
            shadowFactor = lit / 9.0;
        }
    }

    vec3 f0 = mix(vec3(0.04), albedo, metal);
    float ndv = max(dot(n, v), 1e-4);
    vec3 lo = vec3(0.0);
    bool firstDirectional = true;
    for (int i = 0; i < uLightCount; ++i) {
        vec3 l;
        float attenuation = 1.0;
        if (uLightType[i] == 0) {
            l = normalize(-uLightVec[i]);
            if (firstDirectional) {
                attenuation *= mix(0.25, 1.0, shadowFactor);
                firstDirectional = false;
            }
        } else {
            vec3 toLight = uLightVec[i] - vWorldPos;
            float dist = length(toLight);
            l = toLight / max(dist, 1e-4);
            attenuation = clamp(1.0 - dist / uLightRange[i], 0.0, 1.0);
            attenuation *= attenuation;
        }
        vec3 h = normalize(l + v);
        float ndl = max(dot(n, l), 0.0);
        float ndh = max(dot(n, h), 0.0);
        float vdh = max(dot(v, h), 0.0);
        float d = distributionGGX(ndh, rough);
        float g = geometrySchlick(ndv, rough) * geometrySchlick(ndl, rough);
        vec3 f = fresnelSchlick(vdh, f0);
        vec3 spec = (d * g * f) / max(4.0 * ndv * ndl, 1e-4);
        // Diffuse keeps the 1/PI folded into the light, matching the engine's
        // existing exposure (so unlit/scalar materials read the same).
        vec3 kd = (vec3(1.0) - f) * (1.0 - metal);
        vec3 radiance = uLightColor[i] * attenuation;
        lo += (kd * albedo + spec) * radiance * ndl;
    }
    vec3 ambient = albedo * ao * 0.22; // ambient floor, modulated by occlusion
    fragColor = vec4(ambient + lo + uEmissive, 1.0);
}
)glsl";

// --- Renderer ---------------------------------------------------------------

class OpenGlRendererImpl final : public OpenGlRenderer {
public:
    explicit OpenGlRendererImpl(const GlLoader& loader) {
        if (!gl_.load(loader)) {
            return;
        }
        program_ = buildProgram(kVertexShader, kFragmentShader);
        shadowProgram_ = buildProgram(kShadowVertexShader, kShadowFragmentShader);
        if (program_ == 0 || shadowProgram_ == 0) {
            return;
        }
        uSkyMode_ = gl_.GetUniformLocation(program_, "uSkyMode");
        uShadowMap_ = gl_.GetUniformLocation(program_, "uShadowMap");
        uLightSpace_ = gl_.GetUniformLocation(program_, "uLightSpace");
        uHasShadow_ = gl_.GetUniformLocation(program_, "uHasShadow");
        uShadowModel_ = gl_.GetUniformLocation(shadowProgram_, "uModel");
        uShadowLightSpace_ = gl_.GetUniformLocation(shadowProgram_, "uLightSpace");

        // The directional-light shadow map: a depth-only framebuffer.
        gl_.GenTextures(1, &shadowTexture_);
        gl_.BindTexture(GL_TEXTURE_2D, shadowTexture_);
        gl_.TexImage2D(GL_TEXTURE_2D, 0, static_cast<GLint>(GL_DEPTH_COMPONENT24),
                       kShadowMapSize, kShadowMapSize, 0, GL_DEPTH_COMPONENT,
                       GL_FLOAT, nullptr);
        gl_.TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                          static_cast<GLint>(GL_NEAREST));
        gl_.TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER,
                          static_cast<GLint>(GL_NEAREST));
        gl_.TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S,
                          static_cast<GLint>(GL_CLAMP_TO_EDGE));
        gl_.TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T,
                          static_cast<GLint>(GL_CLAMP_TO_EDGE));
        gl_.GenFramebuffers(1, &shadowFbo_);
        gl_.BindFramebuffer(GL_FRAMEBUFFER, shadowFbo_);
        gl_.FramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D,
                                 shadowTexture_, 0);
        gl_.DrawBuffer(GL_NONE_MODE);
        gl_.ReadBuffer(GL_NONE_MODE);
        const bool shadowComplete =
            gl_.CheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE;
        gl_.BindFramebuffer(GL_FRAMEBUFFER, 0);
        if (!shadowComplete) {
            return;
        }
        uModel_ = gl_.GetUniformLocation(program_, "uModel");
        uViewProjection_ = gl_.GetUniformLocation(program_, "uViewProjection");
        uBaseColor_ = gl_.GetUniformLocation(program_, "uBaseColor");
        uEmissive_ = gl_.GetUniformLocation(program_, "uEmissive");
        uRoughness_ = gl_.GetUniformLocation(program_, "uRoughness");
        uMetallic_ = gl_.GetUniformLocation(program_, "uMetallic");
        uCameraPos_ = gl_.GetUniformLocation(program_, "uCameraPos");
        uLightCount_ = gl_.GetUniformLocation(program_, "uLightCount");
        uLightVec_ = gl_.GetUniformLocation(program_, "uLightVec");
        uLightColor_ = gl_.GetUniformLocation(program_, "uLightColor");
        uLightType_ = gl_.GetUniformLocation(program_, "uLightType");
        uLightRange_ = gl_.GetUniformLocation(program_, "uLightRange");
        uTexture_ = gl_.GetUniformLocation(program_, "uTexture");
        uNormalMap_ = gl_.GetUniformLocation(program_, "uNormalMap");
        uRoughnessMap_ = gl_.GetUniformLocation(program_, "uRoughnessMap");
        uMetallicMap_ = gl_.GetUniformLocation(program_, "uMetallicMap");
        uOcclusionMap_ = gl_.GetUniformLocation(program_, "uOcclusionMap");
        uHeightMap_ = gl_.GetUniformLocation(program_, "uHeightMap");
        uUvTiling_ = gl_.GetUniformLocation(program_, "uUvTiling");
        uParallaxDepth_ = gl_.GetUniformLocation(program_, "uParallaxDepth");

        // Neutral defaults bound when a material leaves a PBR slot empty, so
        // the shader can sample every map unconditionally: white is a no-op
        // multiplier (albedo/roughness/metallic/occlusion/height), the flat
        // normal (0.5,0.5,1) decodes to (0,0,1) — no perturbation.
        const std::uint8_t white[4] = {255, 255, 255, 255};
        const std::uint8_t flatNormal[4] = {128, 128, 255, 255};
        whiteTexture_ = makeSolidTexture(white);
        flatNormalTexture_ = makeSolidTexture(flatNormal);

        gl_.GenVertexArrays(1, &cubeVao_);
        gl_.BindVertexArray(cubeVao_);
        GLuint vbo = 0;
        gl_.GenBuffers(1, &vbo);
        gl_.BindBuffer(GL_ARRAY_BUFFER, vbo);
        gl_.BufferData(GL_ARRAY_BUFFER, sizeof(kCubeVertices), kCubeVertices,
                       GL_STATIC_DRAW);
        setupVertexAttributes();
        gl_.BindVertexArray(0);
        ready_ = true;
    }

    /// Engine vertex format: position(3) + normal(3) + uv(2).
    void setupVertexAttributes() {
        constexpr GLsizei kStride = 8 * sizeof(float);
        gl_.EnableVertexAttribArray(0);
        gl_.VertexAttribPointer(0, 3, GL_FLOAT, 0, kStride, nullptr);
        gl_.EnableVertexAttribArray(1);
        gl_.VertexAttribPointer(1, 3, GL_FLOAT, 0, kStride,
                                reinterpret_cast<const void*>(3 * sizeof(float)));
        gl_.EnableVertexAttribArray(2);
        gl_.VertexAttribPointer(2, 2, GL_FLOAT, 0, kStride,
                                reinterpret_cast<const void*>(6 * sizeof(float)));
    }

    /// Binds the texture named by `handleValue` to `unit` (or `defaultTex`
    /// when the handle names no uploaded texture) and points `uniform` at it.
    void bindMap(GLint unit, GLint uniform, std::uint64_t handleValue,
                 GLuint defaultTex) {
        gl_.ActiveTexture(GL_TEXTURE0 + unit);
        const auto it = textures_.find(handleValue);
        gl_.BindTexture(GL_TEXTURE_2D, it != textures_.end() ? it->second : defaultTex);
        gl_.Uniform1i(uniform, unit);
    }

    /// 1x1 RGBA texture used as a neutral default for unbound PBR slots.
    GLuint makeSolidTexture(const std::uint8_t rgba[4]) {
        GLuint texture = 0;
        gl_.GenTextures(1, &texture);
        gl_.BindTexture(GL_TEXTURE_2D, texture);
        gl_.TexImage2D(GL_TEXTURE_2D, 0, static_cast<GLint>(GL_RGBA8), 1, 1, 0,
                       GL_RGBA, GL_UNSIGNED_BYTE, rgba);
        gl_.TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                          static_cast<GLint>(GL_NEAREST));
        gl_.TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER,
                          static_cast<GLint>(GL_NEAREST));
        return texture;
    }

    bool ready() const override { return ready_; }

    // IRenderer

    std::string backendName() const override { return "opengl"; }

    void attachSurface(rendering::IRenderSurface& surface) override {
        surface_ = &surface;
    }

    void submit(std::span<const rendering::RenderCommand> commands) override {
        pending_.insert(pending_.end(), commands.begin(), commands.end());
    }

    void renderFrame() override {
        if (!ready_) {
            pending_.clear();
            return;
        }
        gl_.Enable(GL_DEPTH_TEST);

        // --- Shadow pre-pass: depth from the first directional light ------
        core::Transform cameraPose;
        bool hasCamera = false;
        bool hasSun = false;
        core::Vec3 sunDirection{0.0f, -1.0f, 0.0f};
        std::vector<std::pair<Mat4, std::uint64_t>> shadowDraws;
        for (const auto& command : pending_) {
            if (command.type == rendering::RenderCommandType::SetCamera) {
                cameraPose = command.transform;
                hasCamera = true;
            } else if (command.type == rendering::RenderCommandType::AddLight &&
                       command.lightType == rendering::LightType::Directional &&
                       !hasSun) {
                sunDirection = core::rotate(command.transform.rotation,
                                            {0.0f, 0.0f, -1.0f});
                hasSun = true;
            } else if (command.type == rendering::RenderCommandType::DrawMesh &&
                       command.resource.value != kWireframeResourceId) {
                shadowDraws.emplace_back(fromTransform(command.transform),
                                         command.resource.value);
            }
        }

        Mat4 lightSpace = Mat4::identity();
        const bool shadowsActive = hasSun && hasCamera && !shadowDraws.empty();
        if (shadowsActive) {
            // The shadow box follows the camera's neighbourhood.
            const auto forward = core::rotate(cameraPose.rotation, {0.0f, 0.0f, -1.0f});
            const core::Vec3 centre = cameraPose.position + forward * 12.0f;
            const core::Vec3 eye = centre + sunDirection * -45.0f;
            lightSpace = orthographic(32.0f, 1.0f, 120.0f) *
                         lookAt(eye, centre, {0.0f, 1.0f, 0.0f});

            GLint previousFbo = 0;
            gl_.GetIntegerv(GL_FRAMEBUFFER_BINDING, &previousFbo);
            gl_.BindFramebuffer(GL_FRAMEBUFFER, shadowFbo_);
            gl_.Viewport(0, 0, kShadowMapSize, kShadowMapSize);
            gl_.Clear(GL_DEPTH_BUFFER_BIT);
            gl_.UseProgram(shadowProgram_);
            gl_.UniformMatrix4fv(uShadowLightSpace_, 1, 0, lightSpace.m.data());
            for (const auto& [model, resource] : shadowDraws) {
                gl_.UniformMatrix4fv(uShadowModel_, 1, 0, model.m.data());
                GLuint vao = cubeVao_;
                GLsizei vertexCount = 36;
                if (const auto it = meshes_.find(resource); it != meshes_.end()) {
                    vao = it->second.vao;
                    vertexCount = it->second.vertexCount;
                }
                gl_.BindVertexArray(vao);
                gl_.DrawArrays(GL_TRIANGLES, 0, vertexCount);
            }
            gl_.BindFramebuffer(GL_FRAMEBUFFER, static_cast<GLuint>(previousFbo));
            gl_.Viewport(0, 0, viewportW_, viewportH_);
        }

        gl_.UseProgram(program_);
        gl_.Uniform1i(uHasShadow_, shadowsActive ? 1 : 0);
        if (shadowsActive) {
            gl_.UniformMatrix4fv(uLightSpace_, 1, 0, lightSpace.m.data());
            gl_.ActiveTexture(GL_TEXTURE1);
            gl_.BindTexture(GL_TEXTURE_2D, shadowTexture_);
            gl_.Uniform1i(uShadowMap_, 1);
            gl_.ActiveTexture(GL_TEXTURE0);
        }

        Mat4 viewProjection = Mat4::identity();
        float aspect = 16.0f / 9.0f;
        // Per-frame light set, flushed to uniforms before draws.
        constexpr int kMaxLights = 4;
        GLfloat lightVec[kMaxLights * 3] = {};
        GLfloat lightColor[kMaxLights * 3] = {};
        GLint lightType[kMaxLights] = {};
        GLfloat lightRange[kMaxLights] = {};
        GLint lightCount = 0;
        bool lightsDirty = true;

        const auto flushLights = [&] {
            if (!lightsDirty) {
                return;
            }
            gl_.Uniform1i(uLightCount_, lightCount);
            gl_.Uniform3fv(uLightVec_, kMaxLights, lightVec);
            gl_.Uniform3fv(uLightColor_, kMaxLights, lightColor);
            gl_.Uniform1iv(uLightType_, kMaxLights, lightType);
            gl_.Uniform1fv(uLightRange_, kMaxLights, lightRange);
            lightsDirty = false;
        };

        for (const auto& command : pending_) {
            switch (command.type) {
                case rendering::RenderCommandType::BeginFrame:
                    gl_.ClearColor(0.137f, 0.176f, 0.220f, 1.0f);
                    gl_.Clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
                    lightCount = 0;
                    lightsDirty = true;
                    break;
                case rendering::RenderCommandType::SetViewport:
                    viewportW_ = static_cast<GLsizei>(command.viewportWidth);
                    viewportH_ = static_cast<GLsizei>(command.viewportHeight);
                    gl_.Viewport(0, 0,
                                 static_cast<GLsizei>(command.viewportWidth),
                                 static_cast<GLsizei>(command.viewportHeight));
                    aspect = command.viewportHeight == 0
                                 ? aspect
                                 : static_cast<float>(command.viewportWidth) /
                                       static_cast<float>(command.viewportHeight);
                    break;
                case rendering::RenderCommandType::SetCamera: {
                    Mat4 proj = perspective(command.fovDegrees, aspect, 0.1f, 500.0f);
                    if (command.orthoHeight > 0.0f) {
                        const float h = command.orthoHeight * 0.5f;
                        proj = Mat4::identity();
                        proj.m[0] = 1.0f / (h * aspect);
                        proj.m[5] = 1.0f / h;
                        proj.m[10] = -2.0f / (500.0f - 0.1f);
                        proj.m[14] = -(500.0f + 0.1f) / (500.0f - 0.1f);
                    }
                    viewProjection = proj * viewFromCameraPose(command.transform);
                    gl_.UniformMatrix4fv(uViewProjection_, 1, 0,
                                         viewProjection.m.data());
                    gl_.Uniform3f(uCameraPos_, command.transform.position.x,
                                  command.transform.position.y,
                                  command.transform.position.z);
                    cameraPos_[0] = command.transform.position.x;
                    cameraPos_[1] = command.transform.position.y;
                    cameraPos_[2] = command.transform.position.z;
                    break;
                }
                case rendering::RenderCommandType::AddLight: {
                    if (lightCount >= kMaxLights) {
                        break;
                    }
                    const int slot = lightCount++;
                    // Directional lights shine along their pose's -Z axis;
                    // point lights use the pose position.
                    const auto vec =
                        command.lightType == rendering::LightType::Directional
                            ? core::rotate(command.transform.rotation,
                                           {0.0f, 0.0f, -1.0f})
                            : command.transform.position;
                    lightVec[slot * 3 + 0] = vec.x;
                    lightVec[slot * 3 + 1] = vec.y;
                    lightVec[slot * 3 + 2] = vec.z;
                    lightColor[slot * 3 + 0] = command.color.x * command.lightIntensity;
                    lightColor[slot * 3 + 1] = command.color.y * command.lightIntensity;
                    lightColor[slot * 3 + 2] = command.color.z * command.lightIntensity;
                    lightType[slot] = static_cast<GLint>(command.lightType);
                    lightRange[slot] = command.lightRange;
                    lightsDirty = true;
                    break;
                }
                case rendering::RenderCommandType::SetSky: {
                    // Sky backdrop through the main program: a giant cube
                    // glued to the camera, shaded by view direction.
                    flushLights();
                    core::Transform dome;
                    dome.position = {cameraPos_[0], cameraPos_[1], cameraPos_[2]};
                    dome.scale = {300.0f, 300.0f, 300.0f};
                    const auto model = fromTransform(dome);
                    gl_.UniformMatrix4fv(uModel_, 1, 0, model.m.data());
                    gl_.Uniform3f(uBaseColor_, command.color.x, command.color.y,
                                  command.color.z);
                    gl_.Uniform3f(uEmissive_, command.emissive.x, command.emissive.y,
                                  command.emissive.z);
                    gl_.Uniform1i(uSkyMode_, 1);
                    gl_.Disable(GL_DEPTH_TEST);
                    gl_.DepthMask(0);
                    gl_.BindVertexArray(cubeVao_);
                    gl_.DrawArrays(GL_TRIANGLES, 0, 36);
                    gl_.DepthMask(1);
                    gl_.Enable(GL_DEPTH_TEST);
                    gl_.Uniform1i(uSkyMode_, 0);
                    break;
                }
                case rendering::RenderCommandType::DrawMesh: {
                    flushLights();
                    // Bind every PBR map; absent slots fall back to a neutral
                    // default. Unit 1 is reserved for the shadow map.
                    bindMap(0, uTexture_, command.texture.value, whiteTexture_);
                    bindMap(2, uNormalMap_, command.normalTexture.value,
                            flatNormalTexture_);
                    bindMap(3, uRoughnessMap_, command.roughnessTexture.value,
                            whiteTexture_);
                    bindMap(4, uMetallicMap_, command.metallicTexture.value,
                            whiteTexture_);
                    bindMap(5, uOcclusionMap_, command.occlusionTexture.value,
                            whiteTexture_);
                    bindMap(6, uHeightMap_, command.heightTexture.value,
                            whiteTexture_);
                    gl_.ActiveTexture(GL_TEXTURE0);
                    gl_.Uniform2f(uUvTiling_, command.uvTiling.x, command.uvTiling.y);
                    gl_.Uniform1f(uParallaxDepth_, command.parallaxDepth);
                    const auto model = fromTransform(command.transform);
                    gl_.UniformMatrix4fv(uModel_, 1, 0, model.m.data());
                    gl_.Uniform3f(uBaseColor_, command.color.x, command.color.y,
                                  command.color.z);
                    gl_.Uniform3f(uEmissive_, command.emissive.x, command.emissive.y,
                                  command.emissive.z);
                    gl_.Uniform1f(uRoughness_, command.roughness);
                    gl_.Uniform1f(uMetallic_, command.metallic);
                    // Uploaded mesh if the handle names one, the built-in
                    // cube otherwise.
                    GLuint vao = cubeVao_;
                    GLsizei vertexCount = 36;
                    if (const auto it = meshes_.find(command.resource.value);
                        it != meshes_.end()) {
                        vao = it->second.vao;
                        vertexCount = it->second.vertexCount;
                    }
                    gl_.BindVertexArray(vao);
                    if (command.resource.value == kWireframeResourceId) {
                        gl_.PolygonMode(GL_FRONT_AND_BACK, GL_LINE);
                        gl_.DrawArrays(GL_TRIANGLES, 0, vertexCount);
                        gl_.PolygonMode(GL_FRONT_AND_BACK, GL_FILL);
                    } else {
                        gl_.DrawArrays(GL_TRIANGLES, 0, vertexCount);
                    }
                    break;
                }
                case rendering::RenderCommandType::BindPipeline:
                case rendering::RenderCommandType::EndFrame:
                    break;
            }
        }
        pending_.clear();
        if (surface_ != nullptr) {
            surface_->present();
        }
    }

    // IRenderResourceFactory — the only mesh today is the built-in cube;
    // imported meshes plug in behind the same handles.

    rendering::RenderResourceHandle createFromAsset(
        asset::AssetId assetId, rendering::RenderResourceType) override {
        if (!assetId.isValid()) {
            return rendering::RenderResourceHandle::invalid();
        }
        const rendering::RenderResourceHandle handle{nextResource_++};
        resources_.emplace(handle.value, assetId);
        return handle;
    }

    rendering::RenderResourceHandle createMeshFromData(
        std::span<const float> interleavedPosNormalUv) override {
        if (!ready_ || interleavedPosNormalUv.empty() ||
            interleavedPosNormalUv.size() % 24 != 0) {
            return rendering::RenderResourceHandle::invalid();
        }
        MeshResource mesh;
        mesh.vertexCount =
            static_cast<GLsizei>(interleavedPosNormalUv.size() / 8);
        gl_.GenVertexArrays(1, &mesh.vao);
        gl_.BindVertexArray(mesh.vao);
        GLuint vbo = 0;
        gl_.GenBuffers(1, &vbo);
        gl_.BindBuffer(GL_ARRAY_BUFFER, vbo);
        gl_.BufferData(GL_ARRAY_BUFFER,
                       static_cast<GLsizeiptr>(interleavedPosNormalUv.size() *
                                               sizeof(float)),
                       interleavedPosNormalUv.data(), GL_STATIC_DRAW);
        setupVertexAttributes();
        gl_.BindVertexArray(0);

        const rendering::RenderResourceHandle handle{nextResource_++};
        meshes_.emplace(handle.value, mesh);
        return handle;
    }

    rendering::RenderResourceHandle createTextureFromData(
        std::uint32_t width, std::uint32_t height,
        std::span<const std::uint8_t> rgbaPixels) override {
        if (!ready_ || width == 0 || height == 0 ||
            rgbaPixels.size() != std::size_t(width) * height * 4) {
            return rendering::RenderResourceHandle::invalid();
        }
        GLuint texture = 0;
        gl_.GenTextures(1, &texture);
        gl_.BindTexture(GL_TEXTURE_2D, texture);
        gl_.TexImage2D(GL_TEXTURE_2D, 0, static_cast<GLint>(GL_RGBA8),
                       static_cast<GLsizei>(width), static_cast<GLsizei>(height), 0,
                       GL_RGBA, GL_UNSIGNED_BYTE, rgbaPixels.data());
        gl_.TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                          static_cast<GLint>(GL_LINEAR_MIPMAP_LINEAR));
        gl_.TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER,
                          static_cast<GLint>(GL_LINEAR));
        gl_.TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S,
                          static_cast<GLint>(GL_REPEAT));
        gl_.TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T,
                          static_cast<GLint>(GL_REPEAT));
        gl_.GenerateMipmap(GL_TEXTURE_2D);

        const rendering::RenderResourceHandle handle{nextResource_++};
        textures_.emplace(handle.value, texture);
        return handle;
    }

    void destroy(rendering::RenderResourceHandle resource) override {
        resources_.erase(resource.value);
        meshes_.erase(resource.value);
        if (const auto it = textures_.find(resource.value); it != textures_.end()) {
            gl_.DeleteTextures(1, &it->second);
            textures_.erase(it);
        }
    }

private:
    GLuint buildProgram(const char* vertexSource, const char* fragmentSource) {
        const auto compile = [&](GLenum type, const char* source) -> GLuint {
            const GLuint shader = gl_.CreateShader(type);
            gl_.ShaderSource(shader, 1, &source, nullptr);
            gl_.CompileShader(shader);
            GLint ok = 0;
            gl_.GetShaderiv(shader, GL_COMPILE_STATUS, &ok);
            return ok != 0 ? shader : 0;
        };
        const GLuint vertex = compile(GL_VERTEX_SHADER, vertexSource);
        const GLuint fragment = compile(GL_FRAGMENT_SHADER, fragmentSource);
        if (vertex == 0 || fragment == 0) {
            return 0;
        }
        const GLuint program = gl_.CreateProgram();
        gl_.AttachShader(program, vertex);
        gl_.AttachShader(program, fragment);
        gl_.LinkProgram(program);
        gl_.DeleteShader(vertex);
        gl_.DeleteShader(fragment);
        GLint ok = 0;
        gl_.GetProgramiv(program, GL_LINK_STATUS, &ok);
        return ok != 0 ? program : 0;
    }

    GlApi gl_;
    bool ready_ = false;
    GLuint program_ = 0;
    GLuint cubeVao_ = 0;
    GLint uModel_ = -1;
    GLint uViewProjection_ = -1;
    GLint uBaseColor_ = -1;
    GLint uEmissive_ = -1;
    GLint uRoughness_ = -1;
    GLint uMetallic_ = -1;
    GLint uCameraPos_ = -1;
    GLint uLightCount_ = -1;
    GLint uLightVec_ = -1;
    GLint uLightColor_ = -1;
    GLint uLightType_ = -1;
    GLint uLightRange_ = -1;
    GLint uTexture_ = -1;
    GLint uNormalMap_ = -1;
    GLint uRoughnessMap_ = -1;
    GLint uMetallicMap_ = -1;
    GLint uOcclusionMap_ = -1;
    GLint uHeightMap_ = -1;
    GLint uUvTiling_ = -1;
    GLint uParallaxDepth_ = -1;
    GLuint whiteTexture_ = 0;
    GLuint flatNormalTexture_ = 0;
    GLint uSkyMode_ = -1;
    GLuint shadowProgram_ = 0;
    GLuint shadowFbo_ = 0;
    GLuint shadowTexture_ = 0;
    GLint uShadowMap_ = -1;
    GLint uLightSpace_ = -1;
    GLint uHasShadow_ = -1;
    GLint uShadowModel_ = -1;
    GLint uShadowLightSpace_ = -1;
    GLsizei viewportW_ = 1280;
    GLsizei viewportH_ = 720;
    static constexpr GLsizei kShadowMapSize = 1024;
    float cameraPos_[3] = {0.0f, 0.0f, 0.0f};
    rendering::IRenderSurface* surface_ = nullptr;
    std::vector<rendering::RenderCommand> pending_;
    struct MeshResource {
        GLuint vao = 0;
        GLsizei vertexCount = 0;
    };

    std::uint64_t nextResource_ = 1;
    std::unordered_map<std::uint64_t, asset::AssetId> resources_;
    std::unordered_map<std::uint64_t, MeshResource> meshes_;
    std::unordered_map<std::uint64_t, GLuint> textures_;
};

} // namespace

std::unique_ptr<OpenGlRenderer> createOpenGlRenderer(const GlLoader& loader) {
    return std::make_unique<OpenGlRendererImpl>(loader);
}

void registerOpenGlBackend(rendering::IRendererRegistry& registry) {
    registry.registerBackend(
        "opengl",
        [](const rendering::BackendInit& init) -> std::unique_ptr<rendering::IRenderer> {
            if (init.resolveGlProc == nullptr) {
                return nullptr;
            }
            return createOpenGlRenderer(init.resolveGlProc);
        });
}

} // namespace sky::rendering_opengl
