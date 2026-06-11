#include <unordered_set>
#include <vector>

#include "sky/rendering/null_renderer.hpp"

namespace sky::rendering {
namespace {

class OffscreenSurface final : public IRenderSurface {
public:
    OffscreenSurface(std::uint32_t width, std::uint32_t height)
        : width_(width), height_(height) {}

    std::uint32_t width() const override { return width_; }
    std::uint32_t height() const override { return height_; }
    void present() override {}

private:
    std::uint32_t width_;
    std::uint32_t height_;
};

class NullRendererImpl final : public NullRenderer {
public:
    // IRenderer

    std::string backendName() const override { return "null"; }

    void attachSurface(IRenderSurface& surface) override { surface_ = &surface; }

    void submit(std::span<const RenderCommand> commands) override {
        pending_.insert(pending_.end(), commands.begin(), commands.end());
    }

    void renderFrame() override {
        commandsInLastFrame_ = pending_.size();
        pending_.clear();
        ++frameCount_;
        if (surface_ != nullptr) {
            surface_->present();
        }
    }

    // IRenderResourceFactory

    RenderResourceHandle createFromAsset(asset::AssetId asset,
                                         RenderResourceType /*type*/) override {
        if (!asset.isValid()) {
            return RenderResourceHandle::invalid();
        }
        const RenderResourceHandle handle{nextId_++};
        resources_.insert(handle.value);
        return handle;
    }

    void destroy(RenderResourceHandle resource) override {
        resources_.erase(resource.value);
    }

    // NullRenderer

    std::uint64_t frameCount() const override { return frameCount_; }
    std::size_t commandsInLastFrame() const override { return commandsInLastFrame_; }
    std::size_t liveResourceCount() const override { return resources_.size(); }

private:
    IRenderSurface* surface_ = nullptr;
    std::vector<RenderCommand> pending_;
    std::uint64_t frameCount_ = 0;
    std::size_t commandsInLastFrame_ = 0;
    std::uint64_t nextId_ = 1;
    std::unordered_set<std::uint64_t> resources_;
};

} // namespace

std::unique_ptr<NullRenderer> createNullRenderer() {
    return std::make_unique<NullRendererImpl>();
}

std::unique_ptr<IRenderSurface> createOffscreenSurface(std::uint32_t width,
                                                       std::uint32_t height) {
    return std::make_unique<OffscreenSurface>(width, height);
}

} // namespace sky::rendering
