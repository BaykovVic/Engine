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
               resolve(GenVertexArrays, "glGenVertexArrays") &&
               resolve(BindVertexArray, "glBindVertexArray") &&
               resolve(GenBuffers, "glGenBuffers") &&
               resolve(BindBuffer, "glBindBuffer") &&
               resolve(BufferData, "glBufferData") &&
               resolve(EnableVertexAttribArray, "glEnableVertexAttribArray") &&
               resolve(VertexAttribPointer, "glVertexAttribPointer") &&
               resolve(DrawArrays, "glDrawArrays") &&
               resolve(PolygonMode, "glPolygonMode");
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

// Unit cube centred at the origin: position (3) + normal (3), 36 vertices.
constexpr float kCubeVertices[] = {
    // -Z
    -0.5f,-0.5f,-0.5f, 0,0,-1,  0.5f, 0.5f,-0.5f, 0,0,-1,  0.5f,-0.5f,-0.5f, 0,0,-1,
    -0.5f,-0.5f,-0.5f, 0,0,-1, -0.5f, 0.5f,-0.5f, 0,0,-1,  0.5f, 0.5f,-0.5f, 0,0,-1,
    // +Z
    -0.5f,-0.5f, 0.5f, 0,0,1,   0.5f,-0.5f, 0.5f, 0,0,1,   0.5f, 0.5f, 0.5f, 0,0,1,
    -0.5f,-0.5f, 0.5f, 0,0,1,   0.5f, 0.5f, 0.5f, 0,0,1,  -0.5f, 0.5f, 0.5f, 0,0,1,
    // -X
    -0.5f,-0.5f,-0.5f, -1,0,0, -0.5f,-0.5f, 0.5f, -1,0,0, -0.5f, 0.5f, 0.5f, -1,0,0,
    -0.5f,-0.5f,-0.5f, -1,0,0, -0.5f, 0.5f, 0.5f, -1,0,0, -0.5f, 0.5f,-0.5f, -1,0,0,
    // +X
     0.5f,-0.5f,-0.5f, 1,0,0,   0.5f, 0.5f, 0.5f, 1,0,0,   0.5f,-0.5f, 0.5f, 1,0,0,
     0.5f,-0.5f,-0.5f, 1,0,0,   0.5f, 0.5f,-0.5f, 1,0,0,   0.5f, 0.5f, 0.5f, 1,0,0,
    // -Y
    -0.5f,-0.5f,-0.5f, 0,-1,0,  0.5f,-0.5f,-0.5f, 0,-1,0,  0.5f,-0.5f, 0.5f, 0,-1,0,
    -0.5f,-0.5f,-0.5f, 0,-1,0,  0.5f,-0.5f, 0.5f, 0,-1,0, -0.5f,-0.5f, 0.5f, 0,-1,0,
    // +Y
    -0.5f, 0.5f,-0.5f, 0,1,0,   0.5f, 0.5f, 0.5f, 0,1,0,   0.5f, 0.5f,-0.5f, 0,1,0,
    -0.5f, 0.5f,-0.5f, 0,1,0,  -0.5f, 0.5f, 0.5f, 0,1,0,   0.5f, 0.5f, 0.5f, 0,1,0,
};

const char* kVertexShader = R"glsl(
#version 330 core
layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec3 aNormal;
uniform mat4 uModel;
uniform mat4 uViewProjection;
out vec3 vNormal;
void main() {
    vNormal = mat3(uModel) * aNormal;
    gl_Position = uViewProjection * uModel * vec4(aPosition, 1.0);
}
)glsl";

const char* kFragmentShader = R"glsl(
#version 330 core
in vec3 vNormal;
uniform vec3 uColor;
out vec4 fragColor;
void main() {
    vec3 lightDir = normalize(vec3(0.4, 0.8, 0.45));
    float diffuse = max(dot(normalize(vNormal), lightDir), 0.0);
    vec3 lit = uColor * (0.35 + 0.65 * diffuse);
    fragColor = vec4(lit, 1.0);
}
)glsl";

// --- Renderer ---------------------------------------------------------------

class OpenGlRendererImpl final : public OpenGlRenderer {
public:
    explicit OpenGlRendererImpl(const GlLoader& loader) {
        if (!gl_.load(loader)) {
            return;
        }
        program_ = buildProgram();
        if (program_ == 0) {
            return;
        }
        uModel_ = gl_.GetUniformLocation(program_, "uModel");
        uViewProjection_ = gl_.GetUniformLocation(program_, "uViewProjection");
        uColor_ = gl_.GetUniformLocation(program_, "uColor");

        gl_.GenVertexArrays(1, &cubeVao_);
        gl_.BindVertexArray(cubeVao_);
        GLuint vbo = 0;
        gl_.GenBuffers(1, &vbo);
        gl_.BindBuffer(GL_ARRAY_BUFFER, vbo);
        gl_.BufferData(GL_ARRAY_BUFFER, sizeof(kCubeVertices), kCubeVertices,
                       GL_STATIC_DRAW);
        gl_.EnableVertexAttribArray(0);
        gl_.VertexAttribPointer(0, 3, GL_FLOAT, 0, 6 * sizeof(float), nullptr);
        gl_.EnableVertexAttribArray(1);
        gl_.VertexAttribPointer(1, 3, GL_FLOAT, 0, 6 * sizeof(float),
                                reinterpret_cast<const void*>(3 * sizeof(float)));
        gl_.BindVertexArray(0);
        ready_ = true;
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

        for (const auto& command : pending_) {
            switch (command.type) {
                case rendering::RenderCommandType::BeginFrame:
                    gl_.ClearColor(0.137f, 0.176f, 0.220f, 1.0f);
                    gl_.Clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
                    break;
                case rendering::RenderCommandType::SetViewport:
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
                    break;
                case rendering::RenderCommandType::DrawMesh: {
                    const auto model = fromTransform(command.transform);
                    gl_.UniformMatrix4fv(uModel_, 1, 0, model.m.data());
                    gl_.Uniform3f(uColor_, command.color.x, command.color.y,
                                  command.color.z);
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
        std::span<const float> interleavedPosNormal) override {
        if (!ready_ || interleavedPosNormal.empty() ||
            interleavedPosNormal.size() % 18 != 0) {
            return rendering::RenderResourceHandle::invalid();
        }
        MeshResource mesh;
        mesh.vertexCount =
            static_cast<GLsizei>(interleavedPosNormal.size() / 6);
        gl_.GenVertexArrays(1, &mesh.vao);
        gl_.BindVertexArray(mesh.vao);
        GLuint vbo = 0;
        gl_.GenBuffers(1, &vbo);
        gl_.BindBuffer(GL_ARRAY_BUFFER, vbo);
        gl_.BufferData(GL_ARRAY_BUFFER,
                       static_cast<GLsizeiptr>(interleavedPosNormal.size() *
                                               sizeof(float)),
                       interleavedPosNormal.data(), GL_STATIC_DRAW);
        gl_.EnableVertexAttribArray(0);
        gl_.VertexAttribPointer(0, 3, GL_FLOAT, 0, 6 * sizeof(float), nullptr);
        gl_.EnableVertexAttribArray(1);
        gl_.VertexAttribPointer(1, 3, GL_FLOAT, 0, 6 * sizeof(float),
                                reinterpret_cast<const void*>(3 * sizeof(float)));
        gl_.BindVertexArray(0);

        const rendering::RenderResourceHandle handle{nextResource_++};
        meshes_.emplace(handle.value, mesh);
        return handle;
    }

    void destroy(rendering::RenderResourceHandle resource) override {
        resources_.erase(resource.value);
        meshes_.erase(resource.value);
    }

private:
    GLuint buildProgram() {
        const auto compile = [&](GLenum type, const char* source) -> GLuint {
            const GLuint shader = gl_.CreateShader(type);
            gl_.ShaderSource(shader, 1, &source, nullptr);
            gl_.CompileShader(shader);
            GLint ok = 0;
            gl_.GetShaderiv(shader, GL_COMPILE_STATUS, &ok);
            return ok != 0 ? shader : 0;
        };
        const GLuint vertex = compile(GL_VERTEX_SHADER, kVertexShader);
        const GLuint fragment = compile(GL_FRAGMENT_SHADER, kFragmentShader);
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
    GLint uColor_ = -1;
    rendering::IRenderSurface* surface_ = nullptr;
    std::vector<rendering::RenderCommand> pending_;
    struct MeshResource {
        GLuint vao = 0;
        GLsizei vertexCount = 0;
    };

    std::uint64_t nextResource_ = 1;
    std::unordered_map<std::uint64_t, asset::AssetId> resources_;
    std::unordered_map<std::uint64_t, MeshResource> meshes_;
};

} // namespace

std::unique_ptr<OpenGlRenderer> createOpenGlRenderer(const GlLoader& loader) {
    return std::make_unique<OpenGlRendererImpl>(loader);
}

} // namespace sky::rendering_opengl
