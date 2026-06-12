// Offscreen Vulkan backend: instance -> device -> render pass -> one
// graphics pipeline (the embedded SPIR-V pair) -> command-stream execution
// with host readback. Buffers are host-visible for simplicity; staging and
// swapchain presentation are later increments behind the same contract.

#include <array>
#include <cmath>
#include <cstring>
#include <optional>
#include <unordered_map>

#include <vulkan/vulkan.h>

#include "../shaders/mesh.frag.spv.h"
#include "../shaders/mesh.vert.spv.h"
#include "sky/rendering_vulkan/vulkan_backend.hpp"

namespace sky::rendering_vulkan {
namespace {

constexpr VkFormat kColorFormat = VK_FORMAT_R8G8B8A8_UNORM;
constexpr VkFormat kDepthFormat = VK_FORMAT_D32_SFLOAT;
constexpr int kMaxLights = 4;

// Layout shared with the shaders (std140).
struct FrameUbo {
    float viewProjection[16];
    float cameraPos[4];
    float lightVec[kMaxLights][4];
    float lightColor[kMaxLights][4];
    float lightMeta[kMaxLights][4];
    float counts[4];
};

struct PushBlock {
    float model[16];
    float baseColor[4]; // w = skyMode
    float emissive[4];  // w = roughness
    float params[4];    // x = metallic, y = hasTexture
};

// --- Matrix helpers (column-major, Vulkan clip space) ------------------------

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

/// Vulkan clip space: y points down, depth 0..1.
Mat4 perspective(float fovDegrees, float aspect, float zNear, float zFar) {
    const float f = 1.0f / std::tan(fovDegrees * 3.14159265f / 360.0f);
    Mat4 r;
    r.m[0] = f / aspect;
    r.m[5] = -f;
    r.m[10] = zFar / (zNear - zFar);
    r.m[11] = -1.0f;
    r.m[14] = zFar * zNear / (zNear - zFar);
    return r;
}

Mat4 viewFromCameraPose(const core::Transform& camera) {
    const core::Quat inv{-camera.rotation.x, -camera.rotation.y, -camera.rotation.z,
                         camera.rotation.w};
    core::Transform view;
    view.rotation = inv;
    view.position = core::rotate(inv, {-camera.position.x, -camera.position.y,
                                       -camera.position.z});
    return fromTransform(view);
}

// Unit cube, engine vertex format (position + normal + uv), 36 vertices.
constexpr float kCubeVertices[] = {
    -0.5f,-0.5f,-0.5f, 0,0,-1, 0,0,  0.5f, 0.5f,-0.5f, 0,0,-1, 1,1,  0.5f,-0.5f,-0.5f, 0,0,-1, 1,0,
    -0.5f,-0.5f,-0.5f, 0,0,-1, 0,0, -0.5f, 0.5f,-0.5f, 0,0,-1, 0,1,  0.5f, 0.5f,-0.5f, 0,0,-1, 1,1,
    -0.5f,-0.5f, 0.5f, 0,0,1, 0,0,   0.5f,-0.5f, 0.5f, 0,0,1, 1,0,   0.5f, 0.5f, 0.5f, 0,0,1, 1,1,
    -0.5f,-0.5f, 0.5f, 0,0,1, 0,0,   0.5f, 0.5f, 0.5f, 0,0,1, 1,1,  -0.5f, 0.5f, 0.5f, 0,0,1, 0,1,
    -0.5f,-0.5f,-0.5f, -1,0,0, 0,0, -0.5f,-0.5f, 0.5f, -1,0,0, 1,0, -0.5f, 0.5f, 0.5f, -1,0,0, 1,1,
    -0.5f,-0.5f,-0.5f, -1,0,0, 0,0, -0.5f, 0.5f, 0.5f, -1,0,0, 1,1, -0.5f, 0.5f,-0.5f, -1,0,0, 0,1,
     0.5f,-0.5f,-0.5f, 1,0,0, 0,0,   0.5f, 0.5f, 0.5f, 1,0,0, 1,1,   0.5f,-0.5f, 0.5f, 1,0,0, 1,0,
     0.5f,-0.5f,-0.5f, 1,0,0, 0,0,   0.5f, 0.5f,-0.5f, 1,0,0, 0,1,   0.5f, 0.5f, 0.5f, 1,0,0, 1,1,
    -0.5f,-0.5f,-0.5f, 0,-1,0, 0,0,  0.5f,-0.5f,-0.5f, 0,-1,0, 1,0,  0.5f,-0.5f, 0.5f, 0,-1,0, 1,1,
    -0.5f,-0.5f,-0.5f, 0,-1,0, 0,0,  0.5f,-0.5f, 0.5f, 0,-1,0, 1,1, -0.5f,-0.5f, 0.5f, 0,-1,0, 0,1,
    -0.5f, 0.5f,-0.5f, 0,1,0, 0,0,   0.5f, 0.5f, 0.5f, 0,1,0, 1,1,   0.5f, 0.5f,-0.5f, 0,1,0, 1,0,
    -0.5f, 0.5f,-0.5f, 0,1,0, 0,0,  -0.5f, 0.5f, 0.5f, 0,1,0, 0,1,   0.5f, 0.5f, 0.5f, 0,1,0, 1,1,
};

struct GpuBuffer {
    VkBuffer buffer = VK_NULL_HANDLE;
    VkDeviceMemory memory = VK_NULL_HANDLE;
    std::uint32_t vertexCount = 0;
};

struct GpuTexture {
    VkImage image = VK_NULL_HANDLE;
    VkDeviceMemory memory = VK_NULL_HANDLE;
    VkImageView view = VK_NULL_HANDLE;
    VkDescriptorSet set = VK_NULL_HANDLE;
};

class VulkanRendererImpl final : public VulkanRenderer {
public:
    VulkanRendererImpl(std::uint32_t width, std::uint32_t height)
        : width_(width), height_(height) {
        ready_ = initInstanceAndDevice() && initTarget() && initPipeline() &&
                 initFrameResources();
    }

    ~VulkanRendererImpl() override {
        if (device_ != VK_NULL_HANDLE) {
            vkDeviceWaitIdle(device_);
            for (auto& [id, mesh] : meshes_) {
                destroyBuffer(mesh);
            }
            for (auto& [id, texture] : textures_) {
                destroyTexture(texture);
            }
            destroyTexture(whiteTexture_);
            if (sampler_) vkDestroySampler(device_, sampler_, nullptr);
            if (textureSetLayout_)
                vkDestroyDescriptorSetLayout(device_, textureSetLayout_, nullptr);
            destroyBuffer(cube_);
            destroyBuffer(ubo_);
            destroyBuffer(readback_);
            if (descriptorPool_) vkDestroyDescriptorPool(device_, descriptorPool_, nullptr);
            if (setLayout_) vkDestroyDescriptorSetLayout(device_, setLayout_, nullptr);
            if (pipeline_) vkDestroyPipeline(device_, pipeline_, nullptr);
            if (pipelineLayout_) vkDestroyPipelineLayout(device_, pipelineLayout_, nullptr);
            if (framebuffer_) vkDestroyFramebuffer(device_, framebuffer_, nullptr);
            if (renderPass_) vkDestroyRenderPass(device_, renderPass_, nullptr);
            if (colorView_) vkDestroyImageView(device_, colorView_, nullptr);
            if (depthView_) vkDestroyImageView(device_, depthView_, nullptr);
            if (colorImage_) vkDestroyImage(device_, colorImage_, nullptr);
            if (depthImage_) vkDestroyImage(device_, depthImage_, nullptr);
            if (colorMemory_) vkFreeMemory(device_, colorMemory_, nullptr);
            if (depthMemory_) vkFreeMemory(device_, depthMemory_, nullptr);
            if (commandPool_) vkDestroyCommandPool(device_, commandPool_, nullptr);
            vkDestroyDevice(device_, nullptr);
        }
        if (instance_ != VK_NULL_HANDLE) {
            vkDestroyInstance(instance_, nullptr);
        }
    }

    bool ready() const override { return ready_; }
    std::uint32_t frameWidth() const override { return width_; }
    std::uint32_t frameHeight() const override { return height_; }

    // IRenderer

    std::string backendName() const override { return "vulkan"; }

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
        // First pass over the stream: frame-level state into the UBO.
        FrameUbo frame{};
        Mat4 viewProjection = Mat4::identity();
        float aspect = static_cast<float>(width_) / static_cast<float>(height_);
        int lightCount = 0;
        for (const auto& command : pending_) {
            switch (command.type) {
                case rendering::RenderCommandType::SetViewport:
                    if (command.viewportHeight != 0) {
                        aspect = static_cast<float>(command.viewportWidth) /
                                 static_cast<float>(command.viewportHeight);
                    }
                    break;
                case rendering::RenderCommandType::SetCamera: {
                    viewProjection =
                        perspective(command.fovDegrees, aspect, 0.1f, 500.0f) *
                        viewFromCameraPose(command.transform);
                    frame.cameraPos[0] = command.transform.position.x;
                    frame.cameraPos[1] = command.transform.position.y;
                    frame.cameraPos[2] = command.transform.position.z;
                    break;
                }
                case rendering::RenderCommandType::AddLight: {
                    if (lightCount >= kMaxLights) {
                        break;
                    }
                    const auto vec =
                        command.lightType == rendering::LightType::Directional
                            ? core::rotate(command.transform.rotation,
                                           {0.0f, 0.0f, -1.0f})
                            : command.transform.position;
                    frame.lightVec[lightCount][0] = vec.x;
                    frame.lightVec[lightCount][1] = vec.y;
                    frame.lightVec[lightCount][2] = vec.z;
                    frame.lightColor[lightCount][0] =
                        command.color.x * command.lightIntensity;
                    frame.lightColor[lightCount][1] =
                        command.color.y * command.lightIntensity;
                    frame.lightColor[lightCount][2] =
                        command.color.z * command.lightIntensity;
                    frame.lightMeta[lightCount][0] =
                        command.lightType == rendering::LightType::Point ? 1.0f : 0.0f;
                    frame.lightMeta[lightCount][1] = command.lightRange;
                    ++lightCount;
                    break;
                }
                default:
                    break;
            }
        }
        std::memcpy(frame.viewProjection, viewProjection.m.data(), sizeof(float) * 16);
        frame.counts[0] = static_cast<float>(lightCount);
        std::memcpy(uboMapped_, &frame, sizeof(frame));

        // Record and submit the frame.
        vkResetCommandBuffer(commandBuffer_, 0);
        VkCommandBufferBeginInfo begin{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
        vkBeginCommandBuffer(commandBuffer_, &begin);

        VkClearValue clears[2]{};
        clears[0].color = {{0.137f, 0.176f, 0.220f, 1.0f}};
        clears[1].depthStencil = {1.0f, 0};
        VkRenderPassBeginInfo passBegin{VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
        passBegin.renderPass = renderPass_;
        passBegin.framebuffer = framebuffer_;
        passBegin.renderArea = {{0, 0}, {width_, height_}};
        passBegin.clearValueCount = 2;
        passBegin.pClearValues = clears;
        vkCmdBeginRenderPass(commandBuffer_, &passBegin, VK_SUBPASS_CONTENTS_INLINE);
        vkCmdBindPipeline(commandBuffer_, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_);
        vkCmdBindDescriptorSets(commandBuffer_, VK_PIPELINE_BIND_POINT_GRAPHICS,
                                pipelineLayout_, 0, 1, &descriptorSet_, 0, nullptr);
        const VkViewport viewport{0.0f, 0.0f, static_cast<float>(width_),
                                  static_cast<float>(height_), 0.0f, 1.0f};
        const VkRect2D scissor{{0, 0}, {width_, height_}};
        vkCmdSetViewport(commandBuffer_, 0, 1, &viewport);
        vkCmdSetScissor(commandBuffer_, 0, 1, &scissor);

        for (const auto& command : pending_) {
            switch (command.type) {
                case rendering::RenderCommandType::SetSky: {
                    // The sky dome: a giant cube glued to the camera with
                    // skyMode set in the push constants.
                    core::Transform dome;
                    dome.position = {frame.cameraPos[0], frame.cameraPos[1],
                                     frame.cameraPos[2]};
                    dome.scale = {300.0f, 300.0f, 300.0f};
                    PushBlock push{};
                    std::memcpy(push.model, fromTransform(dome).m.data(),
                                sizeof(push.model));
                    push.baseColor[0] = command.color.x;
                    push.baseColor[1] = command.color.y;
                    push.baseColor[2] = command.color.z;
                    push.baseColor[3] = 1.0f; // skyMode
                    push.emissive[0] = command.emissive.x;
                    push.emissive[1] = command.emissive.y;
                    push.emissive[2] = command.emissive.z;
                    vkCmdBindDescriptorSets(commandBuffer_,
                                            VK_PIPELINE_BIND_POINT_GRAPHICS,
                                            pipelineLayout_, 1, 1,
                                            &whiteTexture_.set, 0, nullptr);
                    drawBuffer(cube_, push);
                    break;
                }
                case rendering::RenderCommandType::DrawMesh: {
                    PushBlock push{};
                    std::memcpy(push.model, fromTransform(command.transform).m.data(),
                                sizeof(push.model));
                    push.baseColor[0] = command.color.x;
                    push.baseColor[1] = command.color.y;
                    push.baseColor[2] = command.color.z;
                    push.emissive[0] = command.emissive.x;
                    push.emissive[1] = command.emissive.y;
                    push.emissive[2] = command.emissive.z;
                    push.emissive[3] = command.roughness;
                    push.params[0] = command.metallic;
                    const auto textureIt = textures_.find(command.texture.value);
                    const auto& texture =
                        textureIt != textures_.end() ? textureIt->second
                                                     : whiteTexture_;
                    push.params[1] = textureIt != textures_.end() ? 1.0f : 0.0f;
                    vkCmdBindDescriptorSets(commandBuffer_,
                                            VK_PIPELINE_BIND_POINT_GRAPHICS,
                                            pipelineLayout_, 1, 1, &texture.set, 0,
                                            nullptr);
                    const auto it = meshes_.find(command.resource.value);
                    drawBuffer(it != meshes_.end() ? it->second : cube_, push);
                    break;
                }
                default:
                    break;
            }
        }

        vkCmdEndRenderPass(commandBuffer_);
        vkEndCommandBuffer(commandBuffer_);

        VkSubmitInfo submitInfo{VK_STRUCTURE_TYPE_SUBMIT_INFO};
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffer_;
        vkQueueSubmit(queue_, 1, &submitInfo, VK_NULL_HANDLE);
        vkQueueWaitIdle(queue_);

        pending_.clear();
        if (surface_ != nullptr) {
            surface_->present();
        }
    }

    // IRenderResourceFactory

    rendering::RenderResourceHandle createFromAsset(
        asset::AssetId assetId, rendering::RenderResourceType) override {
        if (!assetId.isValid()) {
            return rendering::RenderResourceHandle::invalid();
        }
        return rendering::RenderResourceHandle{nextResource_++};
    }

    rendering::RenderResourceHandle createMeshFromData(
        std::span<const float> interleavedPosNormalUv) override {
        if (!ready_ || interleavedPosNormalUv.empty() ||
            interleavedPosNormalUv.size() % 24 != 0) {
            return rendering::RenderResourceHandle::invalid();
        }
        GpuBuffer mesh;
        if (!createVertexBuffer(interleavedPosNormalUv, mesh)) {
            return rendering::RenderResourceHandle::invalid();
        }
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
        auto texture = uploadTexture(width, height, rgbaPixels.data());
        if (texture.set == VK_NULL_HANDLE) {
            return rendering::RenderResourceHandle::invalid();
        }
        const rendering::RenderResourceHandle handle{nextResource_++};
        textures_.emplace(handle.value, texture);
        return handle;
    }

    void destroy(rendering::RenderResourceHandle resource) override {
        if (const auto it = meshes_.find(resource.value); it != meshes_.end()) {
            vkDeviceWaitIdle(device_);
            destroyBuffer(it->second);
            meshes_.erase(it);
        }
        if (const auto it = textures_.find(resource.value); it != textures_.end()) {
            vkDeviceWaitIdle(device_);
            destroyTexture(it->second);
            textures_.erase(it);
        }
    }

    // VulkanRenderer

    std::vector<std::uint8_t> readbackFrame() override {
        if (!ready_) {
            return {};
        }
        // Copy the color image into the host-visible readback buffer.
        vkResetCommandBuffer(commandBuffer_, 0);
        VkCommandBufferBeginInfo begin{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
        vkBeginCommandBuffer(commandBuffer_, &begin);
        VkBufferImageCopy region{};
        region.imageSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1};
        region.imageExtent = {width_, height_, 1};
        vkCmdCopyImageToBuffer(commandBuffer_, colorImage_,
                               VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, readback_.buffer,
                               1, &region);
        vkEndCommandBuffer(commandBuffer_);
        VkSubmitInfo submitInfo{VK_STRUCTURE_TYPE_SUBMIT_INFO};
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffer_;
        vkQueueSubmit(queue_, 1, &submitInfo, VK_NULL_HANDLE);
        vkQueueWaitIdle(queue_);

        std::vector<std::uint8_t> pixels(std::size_t(width_) * height_ * 4);
        std::memcpy(pixels.data(), readbackMapped_, pixels.size());
        return pixels;
    }

private:
    bool initInstanceAndDevice() {
        VkApplicationInfo app{VK_STRUCTURE_TYPE_APPLICATION_INFO};
        app.pApplicationName = "SkyEngine";
        app.apiVersion = VK_API_VERSION_1_1;
        VkInstanceCreateInfo instanceInfo{VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};
        instanceInfo.pApplicationInfo = &app;
        if (vkCreateInstance(&instanceInfo, nullptr, &instance_) != VK_SUCCESS) {
            return false;
        }

        std::uint32_t deviceCount = 0;
        vkEnumeratePhysicalDevices(instance_, &deviceCount, nullptr);
        if (deviceCount == 0) {
            return false;
        }
        std::vector<VkPhysicalDevice> devices(deviceCount);
        vkEnumeratePhysicalDevices(instance_, &deviceCount, devices.data());
        physical_ = devices.front();

        std::uint32_t familyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(physical_, &familyCount, nullptr);
        std::vector<VkQueueFamilyProperties> families(familyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(physical_, &familyCount,
                                                 families.data());
        queueFamily_ = UINT32_MAX;
        for (std::uint32_t i = 0; i < familyCount; ++i) {
            if (families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
                queueFamily_ = i;
                break;
            }
        }
        if (queueFamily_ == UINT32_MAX) {
            return false;
        }

        const float priority = 1.0f;
        VkDeviceQueueCreateInfo queueInfo{VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO};
        queueInfo.queueFamilyIndex = queueFamily_;
        queueInfo.queueCount = 1;
        queueInfo.pQueuePriorities = &priority;
        VkDeviceCreateInfo deviceInfo{VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};
        deviceInfo.queueCreateInfoCount = 1;
        deviceInfo.pQueueCreateInfos = &queueInfo;
        if (vkCreateDevice(physical_, &deviceInfo, nullptr, &device_) != VK_SUCCESS) {
            return false;
        }
        vkGetDeviceQueue(device_, queueFamily_, 0, &queue_);

        VkCommandPoolCreateInfo poolInfo{VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
        poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        poolInfo.queueFamilyIndex = queueFamily_;
        if (vkCreateCommandPool(device_, &poolInfo, nullptr, &commandPool_) !=
            VK_SUCCESS) {
            return false;
        }
        VkCommandBufferAllocateInfo allocInfo{
            VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
        allocInfo.commandPool = commandPool_;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandBufferCount = 1;
        return vkAllocateCommandBuffers(device_, &allocInfo, &commandBuffer_) ==
               VK_SUCCESS;
    }

    std::optional<std::uint32_t> findMemoryType(std::uint32_t typeBits,
                                                VkMemoryPropertyFlags properties) {
        VkPhysicalDeviceMemoryProperties memory;
        vkGetPhysicalDeviceMemoryProperties(physical_, &memory);
        for (std::uint32_t i = 0; i < memory.memoryTypeCount; ++i) {
            if ((typeBits & (1u << i)) != 0 &&
                (memory.memoryTypes[i].propertyFlags & properties) == properties) {
                return i;
            }
        }
        return std::nullopt;
    }

    bool createImage(VkFormat format, VkImageUsageFlags usage,
                     VkImageAspectFlags aspect, VkImage& image,
                     VkDeviceMemory& memory, VkImageView& view) {
        VkImageCreateInfo imageInfo{VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
        imageInfo.imageType = VK_IMAGE_TYPE_2D;
        imageInfo.format = format;
        imageInfo.extent = {width_, height_, 1};
        imageInfo.mipLevels = 1;
        imageInfo.arrayLayers = 1;
        imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        imageInfo.usage = usage;
        if (vkCreateImage(device_, &imageInfo, nullptr, &image) != VK_SUCCESS) {
            return false;
        }
        VkMemoryRequirements requirements;
        vkGetImageMemoryRequirements(device_, image, &requirements);
        const auto type = findMemoryType(requirements.memoryTypeBits,
                                         VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        if (!type) {
            return false;
        }
        VkMemoryAllocateInfo alloc{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
        alloc.allocationSize = requirements.size;
        alloc.memoryTypeIndex = *type;
        if (vkAllocateMemory(device_, &alloc, nullptr, &memory) != VK_SUCCESS ||
            vkBindImageMemory(device_, image, memory, 0) != VK_SUCCESS) {
            return false;
        }
        VkImageViewCreateInfo viewInfo{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
        viewInfo.image = image;
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = format;
        viewInfo.subresourceRange = {aspect, 0, 1, 0, 1};
        return vkCreateImageView(device_, &viewInfo, nullptr, &view) == VK_SUCCESS;
    }

    bool initTarget() {
        if (!createImage(kColorFormat,
                         VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
                             VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
                         VK_IMAGE_ASPECT_COLOR_BIT, colorImage_, colorMemory_,
                         colorView_) ||
            !createImage(kDepthFormat, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
                         VK_IMAGE_ASPECT_DEPTH_BIT, depthImage_, depthMemory_,
                         depthView_)) {
            return false;
        }

        VkAttachmentDescription attachments[2]{};
        attachments[0].format = kColorFormat;
        attachments[0].samples = VK_SAMPLE_COUNT_1_BIT;
        attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        attachments[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        // Left ready for the readback copy after every pass.
        attachments[0].finalLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        attachments[1].format = kDepthFormat;
        attachments[1].samples = VK_SAMPLE_COUNT_1_BIT;
        attachments[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        attachments[1].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachments[1].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        attachments[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        VkAttachmentReference colorRef{0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};
        VkAttachmentReference depthRef{1,
                                       VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL};
        VkSubpassDescription subpass{};
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount = 1;
        subpass.pColorAttachments = &colorRef;
        subpass.pDepthStencilAttachment = &depthRef;

        VkRenderPassCreateInfo passInfo{VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO};
        passInfo.attachmentCount = 2;
        passInfo.pAttachments = attachments;
        passInfo.subpassCount = 1;
        passInfo.pSubpasses = &subpass;
        if (vkCreateRenderPass(device_, &passInfo, nullptr, &renderPass_) !=
            VK_SUCCESS) {
            return false;
        }

        const VkImageView views[2] = {colorView_, depthView_};
        VkFramebufferCreateInfo fbInfo{VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO};
        fbInfo.renderPass = renderPass_;
        fbInfo.attachmentCount = 2;
        fbInfo.pAttachments = views;
        fbInfo.width = width_;
        fbInfo.height = height_;
        fbInfo.layers = 1;
        return vkCreateFramebuffer(device_, &fbInfo, nullptr, &framebuffer_) ==
               VK_SUCCESS;
    }

    VkShaderModule createShader(const std::uint32_t* code, std::size_t words) {
        VkShaderModuleCreateInfo info{VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
        info.codeSize = words * sizeof(std::uint32_t);
        info.pCode = code;
        VkShaderModule module = VK_NULL_HANDLE;
        vkCreateShaderModule(device_, &info, nullptr, &module);
        return module;
    }

    bool initPipeline() {
        VkDescriptorSetLayoutBinding uboBinding{};
        uboBinding.binding = 0;
        uboBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        uboBinding.descriptorCount = 1;
        uboBinding.stageFlags =
            VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
        VkDescriptorSetLayoutCreateInfo setInfo{
            VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
        setInfo.bindingCount = 1;
        setInfo.pBindings = &uboBinding;
        if (vkCreateDescriptorSetLayout(device_, &setInfo, nullptr, &setLayout_) !=
            VK_SUCCESS) {
            return false;
        }

        VkDescriptorSetLayoutBinding samplerBinding{};
        samplerBinding.binding = 0;
        samplerBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        samplerBinding.descriptorCount = 1;
        samplerBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
        VkDescriptorSetLayoutCreateInfo samplerSetInfo{
            VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
        samplerSetInfo.bindingCount = 1;
        samplerSetInfo.pBindings = &samplerBinding;
        if (vkCreateDescriptorSetLayout(device_, &samplerSetInfo, nullptr,
                                        &textureSetLayout_) != VK_SUCCESS) {
            return false;
        }

        VkPushConstantRange pushRange{};
        pushRange.stageFlags =
            VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
        pushRange.size = sizeof(PushBlock);
        const VkDescriptorSetLayout setLayouts[2] = {setLayout_, textureSetLayout_};
        VkPipelineLayoutCreateInfo layoutInfo{
            VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
        layoutInfo.setLayoutCount = 2;
        layoutInfo.pSetLayouts = setLayouts;
        layoutInfo.pushConstantRangeCount = 1;
        layoutInfo.pPushConstantRanges = &pushRange;
        if (vkCreatePipelineLayout(device_, &layoutInfo, nullptr, &pipelineLayout_) !=
            VK_SUCCESS) {
            return false;
        }

        const auto vertex = createShader(k_mesh_vert_spv,
                                         std::size(k_mesh_vert_spv));
        const auto fragment = createShader(k_mesh_frag_spv,
                                           std::size(k_mesh_frag_spv));
        if (vertex == VK_NULL_HANDLE || fragment == VK_NULL_HANDLE) {
            return false;
        }

        VkPipelineShaderStageCreateInfo stages[2]{};
        stages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
        stages[0].module = vertex;
        stages[0].pName = "main";
        stages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        stages[1].module = fragment;
        stages[1].pName = "main";

        VkVertexInputBindingDescription binding{0, 8 * sizeof(float),
                                                VK_VERTEX_INPUT_RATE_VERTEX};
        VkVertexInputAttributeDescription attributes[3]{
            {0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0},
            {1, 0, VK_FORMAT_R32G32B32_SFLOAT, 3 * sizeof(float)},
            {2, 0, VK_FORMAT_R32G32_SFLOAT, 6 * sizeof(float)},
        };
        VkPipelineVertexInputStateCreateInfo vertexInput{
            VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};
        vertexInput.vertexBindingDescriptionCount = 1;
        vertexInput.pVertexBindingDescriptions = &binding;
        vertexInput.vertexAttributeDescriptionCount = 3;
        vertexInput.pVertexAttributeDescriptions = attributes;

        VkPipelineInputAssemblyStateCreateInfo assembly{
            VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO};
        assembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

        VkPipelineViewportStateCreateInfo viewportState{
            VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO};
        viewportState.viewportCount = 1;
        viewportState.scissorCount = 1;

        VkPipelineRasterizationStateCreateInfo raster{
            VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO};
        raster.polygonMode = VK_POLYGON_MODE_FILL;
        raster.cullMode = VK_CULL_MODE_NONE;
        raster.lineWidth = 1.0f;

        VkPipelineMultisampleStateCreateInfo multisample{
            VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO};
        multisample.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

        VkPipelineDepthStencilStateCreateInfo depth{
            VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO};
        depth.depthTestEnable = VK_TRUE;
        depth.depthWriteEnable = VK_TRUE;
        depth.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;

        VkPipelineColorBlendAttachmentState blendAttachment{};
        blendAttachment.colorWriteMask =
            VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
            VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        VkPipelineColorBlendStateCreateInfo blend{
            VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO};
        blend.attachmentCount = 1;
        blend.pAttachments = &blendAttachment;

        const VkDynamicState dynamicStates[] = {VK_DYNAMIC_STATE_VIEWPORT,
                                                VK_DYNAMIC_STATE_SCISSOR};
        VkPipelineDynamicStateCreateInfo dynamic{
            VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO};
        dynamic.dynamicStateCount = 2;
        dynamic.pDynamicStates = dynamicStates;

        VkGraphicsPipelineCreateInfo pipelineInfo{
            VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO};
        pipelineInfo.stageCount = 2;
        pipelineInfo.pStages = stages;
        pipelineInfo.pVertexInputState = &vertexInput;
        pipelineInfo.pInputAssemblyState = &assembly;
        pipelineInfo.pViewportState = &viewportState;
        pipelineInfo.pRasterizationState = &raster;
        pipelineInfo.pMultisampleState = &multisample;
        pipelineInfo.pDepthStencilState = &depth;
        pipelineInfo.pColorBlendState = &blend;
        pipelineInfo.pDynamicState = &dynamic;
        pipelineInfo.layout = pipelineLayout_;
        pipelineInfo.renderPass = renderPass_;
        const auto result = vkCreateGraphicsPipelines(
            device_, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline_);
        vkDestroyShaderModule(device_, vertex, nullptr);
        vkDestroyShaderModule(device_, fragment, nullptr);
        return result == VK_SUCCESS;
    }

    bool createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, GpuBuffer& out,
                      void** mapped) {
        VkBufferCreateInfo bufferInfo{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
        bufferInfo.size = size;
        bufferInfo.usage = usage;
        if (vkCreateBuffer(device_, &bufferInfo, nullptr, &out.buffer) != VK_SUCCESS) {
            return false;
        }
        VkMemoryRequirements requirements;
        vkGetBufferMemoryRequirements(device_, out.buffer, &requirements);
        const auto type =
            findMemoryType(requirements.memoryTypeBits,
                           VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                               VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
        if (!type) {
            return false;
        }
        VkMemoryAllocateInfo alloc{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
        alloc.allocationSize = requirements.size;
        alloc.memoryTypeIndex = *type;
        if (vkAllocateMemory(device_, &alloc, nullptr, &out.memory) != VK_SUCCESS ||
            vkBindBufferMemory(device_, out.buffer, out.memory, 0) != VK_SUCCESS) {
            return false;
        }
        return mapped == nullptr ||
               vkMapMemory(device_, out.memory, 0, size, 0, mapped) == VK_SUCCESS;
    }

    bool createVertexBuffer(std::span<const float> vertices, GpuBuffer& out) {
        void* mapped = nullptr;
        if (!createBuffer(vertices.size() * sizeof(float),
                          VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, out, &mapped)) {
            return false;
        }
        std::memcpy(mapped, vertices.data(), vertices.size() * sizeof(float));
        vkUnmapMemory(device_, out.memory);
        out.vertexCount = static_cast<std::uint32_t>(vertices.size() / 8);
        return true;
    }

    bool initFrameResources() {
        if (!createVertexBuffer(kCubeVertices, cube_) ||
            !createBuffer(sizeof(FrameUbo), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, ubo_,
                          &uboMapped_) ||
            !createBuffer(VkDeviceSize(width_) * height_ * 4,
                          VK_BUFFER_USAGE_TRANSFER_DST_BIT, readback_,
                          &readbackMapped_)) {
            return false;
        }

        const VkDescriptorPoolSize poolSizes[2] = {
            {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1},
            {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 256},
        };
        VkDescriptorPoolCreateInfo poolInfo{
            VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
        poolInfo.maxSets = 257;
        poolInfo.poolSizeCount = 2;
        poolInfo.pPoolSizes = poolSizes;
        if (vkCreateDescriptorPool(device_, &poolInfo, nullptr, &descriptorPool_) !=
            VK_SUCCESS) {
            return false;
        }
        VkDescriptorSetAllocateInfo setAlloc{
            VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
        setAlloc.descriptorPool = descriptorPool_;
        setAlloc.descriptorSetCount = 1;
        setAlloc.pSetLayouts = &setLayout_;
        if (vkAllocateDescriptorSets(device_, &setAlloc, &descriptorSet_) !=
            VK_SUCCESS) {
            return false;
        }
        VkDescriptorBufferInfo bufferInfo{ubo_.buffer, 0, sizeof(FrameUbo)};
        VkWriteDescriptorSet write{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
        write.dstSet = descriptorSet_;
        write.descriptorCount = 1;
        write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        write.pBufferInfo = &bufferInfo;
        vkUpdateDescriptorSets(device_, 1, &write, 0, nullptr);

        VkSamplerCreateInfo samplerInfo{VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};
        samplerInfo.magFilter = VK_FILTER_LINEAR;
        samplerInfo.minFilter = VK_FILTER_LINEAR;
        samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        if (vkCreateSampler(device_, &samplerInfo, nullptr, &sampler_) != VK_SUCCESS) {
            return false;
        }

        // Untextured draws bind a 1x1 white texture so the pipeline layout
        // is always complete.
        const std::uint8_t white[4] = {255, 255, 255, 255};
        whiteTexture_ = uploadTexture(1, 1, white);
        return whiteTexture_.set != VK_NULL_HANDLE;
    }

    GpuTexture uploadTexture(std::uint32_t width, std::uint32_t height,
                             const std::uint8_t* rgbaPixels) {
        GpuTexture texture;
        const VkDeviceSize size = VkDeviceSize(width) * height * 4;

        GpuBuffer staging;
        void* mapped = nullptr;
        if (!createBuffer(size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, staging, &mapped)) {
            return texture;
        }
        std::memcpy(mapped, rgbaPixels, size);
        vkUnmapMemory(device_, staging.memory);

        VkImageCreateInfo imageInfo{VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
        imageInfo.imageType = VK_IMAGE_TYPE_2D;
        imageInfo.format = kColorFormat;
        imageInfo.extent = {width, height, 1};
        imageInfo.mipLevels = 1;
        imageInfo.arrayLayers = 1;
        imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        imageInfo.usage =
            VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
        if (vkCreateImage(device_, &imageInfo, nullptr, &texture.image) != VK_SUCCESS) {
            destroyBuffer(staging);
            return texture;
        }
        VkMemoryRequirements requirements;
        vkGetImageMemoryRequirements(device_, texture.image, &requirements);
        const auto type = findMemoryType(requirements.memoryTypeBits,
                                         VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        VkMemoryAllocateInfo alloc{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
        alloc.allocationSize = requirements.size;
        alloc.memoryTypeIndex = type.value_or(0);
        if (!type ||
            vkAllocateMemory(device_, &alloc, nullptr, &texture.memory) != VK_SUCCESS ||
            vkBindImageMemory(device_, texture.image, texture.memory, 0) !=
                VK_SUCCESS) {
            destroyBuffer(staging);
            destroyTexture(texture);
            return {};
        }

        // One-shot copy with the two layout transitions around it.
        vkResetCommandBuffer(commandBuffer_, 0);
        VkCommandBufferBeginInfo begin{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
        vkBeginCommandBuffer(commandBuffer_, &begin);

        VkImageMemoryBarrier toDst{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
        toDst.srcAccessMask = 0;
        toDst.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        toDst.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        toDst.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        toDst.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        toDst.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        toDst.image = texture.image;
        toDst.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
        vkCmdPipelineBarrier(commandBuffer_, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                             VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr,
                             1, &toDst);

        VkBufferImageCopy region{};
        region.imageSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1};
        region.imageExtent = {width, height, 1};
        vkCmdCopyBufferToImage(commandBuffer_, staging.buffer, texture.image,
                               VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

        VkImageMemoryBarrier toRead = toDst;
        toRead.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        toRead.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        toRead.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        toRead.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        vkCmdPipelineBarrier(commandBuffer_, VK_PIPELINE_STAGE_TRANSFER_BIT,
                             VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0,
                             nullptr, 1, &toRead);

        vkEndCommandBuffer(commandBuffer_);
        VkSubmitInfo submitInfo{VK_STRUCTURE_TYPE_SUBMIT_INFO};
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffer_;
        vkQueueSubmit(queue_, 1, &submitInfo, VK_NULL_HANDLE);
        vkQueueWaitIdle(queue_);
        destroyBuffer(staging);

        VkImageViewCreateInfo viewInfo{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
        viewInfo.image = texture.image;
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = kColorFormat;
        viewInfo.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
        if (vkCreateImageView(device_, &viewInfo, nullptr, &texture.view) !=
            VK_SUCCESS) {
            destroyTexture(texture);
            return {};
        }

        VkDescriptorSetAllocateInfo setAlloc{
            VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
        setAlloc.descriptorPool = descriptorPool_;
        setAlloc.descriptorSetCount = 1;
        setAlloc.pSetLayouts = &textureSetLayout_;
        if (vkAllocateDescriptorSets(device_, &setAlloc, &texture.set) != VK_SUCCESS) {
            destroyTexture(texture);
            return {};
        }
        VkDescriptorImageInfo imageDescriptor{
            sampler_, texture.view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
        VkWriteDescriptorSet samplerWrite{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
        samplerWrite.dstSet = texture.set;
        samplerWrite.descriptorCount = 1;
        samplerWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        samplerWrite.pImageInfo = &imageDescriptor;
        vkUpdateDescriptorSets(device_, 1, &samplerWrite, 0, nullptr);
        return texture;
    }

    void destroyTexture(GpuTexture& texture) {
        if (texture.view) vkDestroyImageView(device_, texture.view, nullptr);
        if (texture.image) vkDestroyImage(device_, texture.image, nullptr);
        if (texture.memory) vkFreeMemory(device_, texture.memory, nullptr);
        texture = {};
    }

    void drawBuffer(const GpuBuffer& mesh, const PushBlock& push) {
        vkCmdPushConstants(commandBuffer_, pipelineLayout_,
                           VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                           0, sizeof(PushBlock), &push);
        const VkDeviceSize offset = 0;
        vkCmdBindVertexBuffers(commandBuffer_, 0, 1, &mesh.buffer, &offset);
        vkCmdDraw(commandBuffer_, mesh.vertexCount, 1, 0, 0);
    }

    void destroyBuffer(GpuBuffer& buffer) {
        if (buffer.buffer) {
            vkDestroyBuffer(device_, buffer.buffer, nullptr);
        }
        if (buffer.memory) {
            vkFreeMemory(device_, buffer.memory, nullptr);
        }
        buffer = {};
    }

    std::uint32_t width_;
    std::uint32_t height_;
    bool ready_ = false;

    VkInstance instance_ = VK_NULL_HANDLE;
    VkPhysicalDevice physical_ = VK_NULL_HANDLE;
    std::uint32_t queueFamily_ = 0;
    VkDevice device_ = VK_NULL_HANDLE;
    VkQueue queue_ = VK_NULL_HANDLE;
    VkCommandPool commandPool_ = VK_NULL_HANDLE;
    VkCommandBuffer commandBuffer_ = VK_NULL_HANDLE;

    VkImage colorImage_ = VK_NULL_HANDLE;
    VkDeviceMemory colorMemory_ = VK_NULL_HANDLE;
    VkImageView colorView_ = VK_NULL_HANDLE;
    VkImage depthImage_ = VK_NULL_HANDLE;
    VkDeviceMemory depthMemory_ = VK_NULL_HANDLE;
    VkImageView depthView_ = VK_NULL_HANDLE;
    VkRenderPass renderPass_ = VK_NULL_HANDLE;
    VkFramebuffer framebuffer_ = VK_NULL_HANDLE;

    VkDescriptorSetLayout setLayout_ = VK_NULL_HANDLE;
    VkPipelineLayout pipelineLayout_ = VK_NULL_HANDLE;
    VkPipeline pipeline_ = VK_NULL_HANDLE;
    VkDescriptorPool descriptorPool_ = VK_NULL_HANDLE;
    VkDescriptorSet descriptorSet_ = VK_NULL_HANDLE;

    GpuBuffer cube_;
    GpuBuffer ubo_;
    GpuBuffer readback_;
    VkSampler sampler_ = VK_NULL_HANDLE;
    VkDescriptorSetLayout textureSetLayout_ = VK_NULL_HANDLE;
    GpuTexture whiteTexture_;
    std::unordered_map<std::uint64_t, GpuTexture> textures_;
    void* uboMapped_ = nullptr;
    void* readbackMapped_ = nullptr;

    rendering::IRenderSurface* surface_ = nullptr;
    std::vector<rendering::RenderCommand> pending_;
    std::uint64_t nextResource_ = 1;
    std::unordered_map<std::uint64_t, GpuBuffer> meshes_;
};

} // namespace

std::unique_ptr<VulkanRenderer> createVulkanRenderer(std::uint32_t width,
                                                     std::uint32_t height) {
    auto renderer = std::make_unique<VulkanRendererImpl>(width, height);
    return renderer->ready() ? std::move(renderer) : nullptr;
}

void registerVulkanBackend(rendering::IRendererRegistry& registry,
                           std::uint32_t width, std::uint32_t height) {
    registry.registerBackend(
        "vulkan",
        [width, height](const rendering::BackendInit&)
            -> std::unique_ptr<rendering::IRenderer> {
            return createVulkanRenderer(width, height);
        });
}

} // namespace sky::rendering_vulkan
