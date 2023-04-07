#pragma once

#include "sve_device.hpp"
#include "sve_game_object.hpp"
#include "sve_pipeline.hpp"
#include "sve_swap_chain.hpp"
#include "sve_window.hpp"

// std
#include <memory>
#include <vector>

namespace sve {
class FirstApp {
   public:
    static constexpr int WIDTH = 800;
    static constexpr int HEIGHT = 600;

    FirstApp();
    ~FirstApp();

    FirstApp(const FirstApp &) = delete;
    FirstApp &operator=(const FirstApp &) = delete;

    void run();

   private:
    void loadGameObjects();
    void createPipelineLayout();
    void createPipeline();
    void createCommandBuffers();
    void freeCommandBuffers();
    void drawFrame();
    void recreateSwapChain();
    void recordCommandBuffer(int imageIndex);
    void renderGameObjects(VkCommandBuffer commandBuffer);

    SveWindow sveWindow{WIDTH, HEIGHT, "Funky Animation"};
    SveDevice sveDevice{sveWindow};
    std::unique_ptr<SveSwapChain> sveSwapChain;
    std::unique_ptr<SvePipeline> svePipeline;
    VkPipelineLayout pipelineLayout;
    std::vector<VkCommandBuffer> commandBuffers;
    std::vector<SveGameObject> gameObjects;
};

}  // namespace sve