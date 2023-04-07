#include "first_app.hpp"

// libs
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>  // for PI

// std
#include <stdexcept>

namespace sve {

struct SimplePushConstantData {
    glm::mat2 transform{1.f};
    glm::vec2 offset;
    alignas(16) glm::vec3 color;
};

FirstApp::FirstApp() {
    loadGameObjects();
    createPipelineLayout();
    recreateSwapChain();
    createCommandBuffers();
}

FirstApp::~FirstApp() {
    vkDestroyPipelineLayout(sveDevice.device(), pipelineLayout, nullptr);
}

void FirstApp::run() {
    while (!sveWindow.shouldClose()) {
        glfwPollEvents();
        drawFrame();
    }
    vkDeviceWaitIdle(sveDevice.device());
}

void FirstApp::loadGameObjects() {
    std::vector<SveModel::Vertex> vertices{
        {{-0.5f, 0.5f}, {1.0f, 0.0f, 0.0f}},
        {{0.5f, 0.5f}, {0.0f, 1.0f, 0.0f}},
        {{0.0f, -0.5f}, {0.0f, 0.0f, 1.0f}}};
    auto sveModel = std::make_shared<SveModel>(sveDevice, vertices);

    std::vector<glm::vec3> colors{
        {1.f, .7f, .73f},
        {1.f, .87f, .73f},
        {1.f, 1.f, .73f},
        {.73f, 1.f, .8f},
        {.73, .88f, 1.f}};

    for (auto& color : colors) {
        color = glm::pow(color, glm::vec3{2.2f});
    }
    for (int i = 0; i < 40; i++) {
        auto triangle = SveGameObject::createGameObject();
        triangle.model = sveModel;
        triangle.transform2d.scale = glm::vec2(.5f) + i * 0.025f;
        triangle.transform2d.rotation = i * glm::pi<float>() * .025f;
        triangle.color = colors[i % colors.size()];
        gameObjects.push_back(std::move(triangle));
    }
}

void FirstApp::createPipelineLayout() {
    // push constant
    VkPushConstantRange pushConstantRange{};
    pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    pushConstantRange.size = sizeof(SimplePushConstantData);

    // pipeline info
    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 0;
    pipelineLayoutInfo.pSetLayouts = nullptr;
    pipelineLayoutInfo.pushConstantRangeCount = 1;
    pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;

    if (vkCreatePipelineLayout(sveDevice.device(), &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS) {
        throw std::runtime_error("failed to create pipeline layout!");
    }
}

void FirstApp::createPipeline() {
    assert(sveSwapChain != nullptr && "Cannot create pipeline before swap chain");
    assert(pipelineLayout != nullptr && "Cannot create pipeline before pipeline layout");

    PipelineConfigInfo pipelineConfig{};
    SvePipeline::defaultPipelineConfigInfo(pipelineConfig);
    pipelineConfig.renderPass = sveSwapChain->getRenderPass();
    pipelineConfig.pipelineLayout = pipelineLayout;
    svePipeline = std::make_unique<SvePipeline>(
        sveDevice,
        "shaders/simple_shader.vert.spv",
        "shaders/simple_shader.frag.spv",
        pipelineConfig);
}

void FirstApp::recreateSwapChain() {
    auto extent = sveWindow.getExtent();
    while (extent.width == 0 || extent.height == 0) {
        extent = sveWindow.getExtent();
        glfwWaitEvents();
    }

    vkDeviceWaitIdle(sveDevice.device());

    if (sveSwapChain == nullptr) {
        sveSwapChain = std::make_unique<SveSwapChain>(sveDevice, extent);
    } else {
        sveSwapChain = std::make_unique<SveSwapChain>(sveDevice, extent, std::move(sveSwapChain));
        if (sveSwapChain->imageCount() != commandBuffers.size()) {
            freeCommandBuffers();
            createCommandBuffers();
        }
    }

    // if render pass compatible, no need to recreate
    createPipeline();
}

void FirstApp::createCommandBuffers() {
    commandBuffers.resize(sveSwapChain->imageCount());

    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = sveDevice.getCommandPool();
    allocInfo.commandBufferCount = static_cast<uint32_t>(commandBuffers.size());

    if (vkAllocateCommandBuffers(sveDevice.device(), &allocInfo, commandBuffers.data()) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate command buffers!");
    }
}

void FirstApp::freeCommandBuffers() {
    vkFreeCommandBuffers(
        sveDevice.device(),
        sveDevice.getCommandPool(),
        static_cast<uint32_t>(commandBuffers.size()),
        commandBuffers.data());
    commandBuffers.clear();
}

void FirstApp::recordCommandBuffer(int imageIndex) {
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = 0;                   // Optional
    beginInfo.pInheritanceInfo = nullptr;  // Optional

    if (vkBeginCommandBuffer(commandBuffers[imageIndex], &beginInfo) != VK_SUCCESS) {
        throw std::runtime_error("failed to begin recording command buffer!");
    }

    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = sveSwapChain->getRenderPass();
    renderPassInfo.framebuffer = sveSwapChain->getFrameBuffer(imageIndex);
    renderPassInfo.renderArea.offset = {0, 0};
    renderPassInfo.renderArea.extent = sveSwapChain->getSwapChainExtent();

    // Attachment index specified in the renderpass
    std::array<VkClearValue, 2> clearValues{};
    clearValues[0].color = {0.01f, 0.01f, 0.01f, 1.0f};  // background color
    clearValues[1].depthStencil = {1.0f, 0};

    renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
    renderPassInfo.pClearValues = clearValues.data();

    vkCmdBeginRenderPass(commandBuffers[imageIndex], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 00.f;
    viewport.width = static_cast<float>(sveSwapChain->getSwapChainExtent().width);
    viewport.height = static_cast<float>(sveSwapChain->getSwapChainExtent().height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    VkRect2D scissor{{0, 0}, sveSwapChain->getSwapChainExtent()};
    vkCmdSetViewport(commandBuffers[imageIndex], 0, 1, &viewport);
    vkCmdSetScissor(commandBuffers[imageIndex], 0, 1, &scissor);

    renderGameObjects(commandBuffers[imageIndex]);

    vkCmdEndRenderPass(commandBuffers[imageIndex]);
    if (vkEndCommandBuffer(commandBuffers[imageIndex]) != VK_SUCCESS) {
        throw std::runtime_error("failed to record command buffer!");
    }
}

void FirstApp::renderGameObjects(VkCommandBuffer commandBuffer) {
    // update
    int i = 0;
    for (auto& obj : gameObjects) {
        i += 1;
        obj.transform2d.rotation = glm::mod<float>(obj.transform2d.rotation + 0.00005f * i, 2.f * glm::pi<float>());
    }

    // render
    svePipeline->bind(commandBuffer);
    for (auto& obj : gameObjects) {
        SimplePushConstantData push{};
        push.offset = obj.transform2d.translation;
        push.color = obj.color;
        push.transform = obj.transform2d.mat2();

        vkCmdPushConstants(
            commandBuffer,
            pipelineLayout,
            VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
            0,
            sizeof(SimplePushConstantData),
            &push);
        obj.model->bind(commandBuffer);
        obj.model->draw(commandBuffer);
    }
}

void FirstApp::drawFrame() {
    uint32_t imageIndex;
    auto result = sveSwapChain->acquireNextImage(&imageIndex);

    // check if window has been resized and swapchain is still valid
    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        recreateSwapChain();
        return;
    }

    // swapchain validity check
    if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        throw std::runtime_error("failed to acquire swap chain image!");
    }

    // record & submit command buffer
    recordCommandBuffer(imageIndex);
    result = sveSwapChain->submitCommandBuffers(&commandBuffers[imageIndex], &imageIndex);

    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || sveWindow.wasWindowResized()) {
        sveWindow.resetWindowResizedFlag();
        recreateSwapChain();
        return;
    }
    if (result != VK_SUCCESS) {
        throw std::runtime_error("failed to present swap chain image!");
    }
}

}  // namespace sve