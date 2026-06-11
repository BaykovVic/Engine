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
               resolve(DepthFunc, "glDepthFunc");
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

// Blinn-Phong with material parameters and up to 4 scene lights.
const char* kFragmentShader = R"glsl(
#version 330 core
in vec3 vNormal;
in vec3 vWorldPos;
in vec2 vUv;
uniform vec3 uBaseColor;
uniform sampler2D uTexture;
uniform int uHasTexture;
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
    vec3 n = normalize(vNormal);
    vec3 v = normalize(uCameraPos - vWorldPos);
    vec3 albedo = uBaseColor;
    if (uHasTexture == 1) {
        albedo *= texture(uTexture, vUv).rgb;
    }
    vec3 result = albedo * 0.22; // ambient floor
    for (int i = 0; i < uLightCount; ++i) {
        vec3 l;
        float attenuation = 1.0;
        if (uLightType[i] == 0) {
            l = normalize(-uLightVec[i]);
        } else {
            vec3 toLight = uLightVec[i] - vWorldPos;
            float dist = length(toLight);
            l = toLight / max(dist, 1e-4);
            attenuation = clamp(1.0 - dist / uLightRange[i], 0.0, 1.0);
            attenuation *= attenuation;
        }
        float ndl = max(dot(n, l), 0.0);
        vec3 h = normalize(l + v);
        float shininess = mix(96.0, 4.0, uRoughness);
        float spec = pow(max(dot(n, h), 0.0), shininess) * (1.0 - uRoughness * 0.7);
        vec3 diffuse = albedo * (1.0 - uMetallic);
        vec3 specColor = mix(vec3(0.04), albedo, uMetallic);
        result += (diffuse * ndl + specColor * spec * ndl) *
                  uLightColor[i] * attenuation;
    }
    result += uEmissive;
    fragColor = vec4(result, 1.0);
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
        if (program_ == 0) {
            return;
        }
        uSkyMode_ = gl_.GetUniformLocation(program_, "uSkyMode");
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
        uHasTexture_ = gl_.GetUniformLocation(program_, "uHasTexture");

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
        gl_.UseProgram(program_);

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
                case rendering::RenderCommandType::SetCamera:
                    viewProjection =
                        perspective(command.fovDegrees, aspect, 0.1f, 500.0f) *
                        viewFromCameraPose(command.transform);
                    gl_.UniformMatrix4fv(uViewProjection_, 1, 0,
                                         viewProjection.m.data());
                    gl_.Uniform3f(uCameraPos_, command.transform.position.x,
                                  command.transform.position.y,
                                  command.transform.position.z);
                    cameraPos_[0] = command.transform.position.x;
                    cameraPos_[1] = command.transform.position.y;
                    cameraPos_[2] = command.transform.position.z;
                    break;
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
                    // Albedo texture, when the command names one.
                    if (const auto it = textures_.find(command.texture.value);
                        it != textures_.end()) {
                        gl_.ActiveTexture(GL_TEXTURE0);
                        gl_.BindTexture(GL_TEXTURE_2D, it->second);
                        gl_.Uniform1i(uTexture_, 0);
                        gl_.Uniform1i(uHasTexture_, 1);
                    } else {
                        gl_.Uniform1i(uHasTexture_, 0);
                    }
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
    GLint uHasTexture_ = -1;
    GLint uSkyMode_ = -1;
    float cameraPos_[3] = {0.0f, 0.0f, 0.0f};
    GLsizei viewportW_ = 0;
    GLsizei viewportH_ = 0;
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
