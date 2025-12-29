#include "vulkan_rendering_device.hpp"

#include "vulkan_rendering_pipeline.hpp"
#include "vulkan_rendering_window.hpp"

#include <iostream>
#include <sstream>

#if defined(_WIN64)
#define NOMINMAX
#include <Windows.h>
#endif

PFN_vkCreateDebugUtilsMessengerEXT pfnVkCreateDebugUtilsMessengerEXT;
PFN_vkDestroyDebugUtilsMessengerEXT pfnVkDestroyDebugUtilsMessengerEXT;

VKAPI_ATTR VkResult VKAPI_CALL vkCreateDebugUtilsMessengerEXT(VkInstance instance,
                                                              const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
                                                              const VkAllocationCallbacks* pAllocator,
                                                              VkDebugUtilsMessengerEXT* pMessenger) {
    return pfnVkCreateDebugUtilsMessengerEXT(instance, pCreateInfo, pAllocator, pMessenger);
}

VKAPI_ATTR void VKAPI_CALL vkDestroyDebugUtilsMessengerEXT(VkInstance instance,
                                                           VkDebugUtilsMessengerEXT messenger,
                                                           VkAllocationCallbacks const* pAllocator) {
    return pfnVkDestroyDebugUtilsMessengerEXT(instance, messenger, pAllocator);
}

namespace broken {

static VKAPI_ATTR vk::Bool32 VKAPI_CALL
vulkanDebugMessengerCallback(vk::DebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                             vk::DebugUtilsMessageTypeFlagsEXT messageTypes,
                             vk::DebugUtilsMessengerCallbackDataEXT const* pCallbackData,
                             void* /*pUserData*/) {
    std::ostringstream message;

    message << vk::to_string(messageSeverity) << ": " << vk::to_string(messageTypes) << ":\n";
    message << std::string("\t") << "messageIDName   = <" << pCallbackData->pMessageIdName << ">\n";
    message << std::string("\t") << "messageIdNumber = " << pCallbackData->messageIdNumber << "\n";
    message << std::string("\t") << "message         = <" << pCallbackData->pMessage << ">\n";
    if (0 < pCallbackData->queueLabelCount) {
        message << std::string("\t") << "Queue Labels:\n";
        for (uint32_t i = 0; i < pCallbackData->queueLabelCount; i++) {
            message << std::string("\t\t") << "labelName = <" << pCallbackData->pQueueLabels[i].pLabelName << ">\n";
        }
    }
    if (0 < pCallbackData->cmdBufLabelCount) {
        message << std::string("\t") << "CommandBuffer Labels:\n";
        for (uint32_t i = 0; i < pCallbackData->cmdBufLabelCount; i++) {
            message << std::string("\t\t") << "labelName = <" << pCallbackData->pCmdBufLabels[i].pLabelName << ">\n";
        }
    }
    if (0 < pCallbackData->objectCount) {
        message << std::string("\t") << "Objects:\n";
        for (uint32_t i = 0; i < pCallbackData->objectCount; i++) {
            message << std::string("\t\t") << "Object " << i << "\n";
            message << std::string("\t\t\t")
                    << "objectType   = " << vk::to_string(pCallbackData->pObjects[i].objectType) << "\n";
            message << std::string("\t\t\t") << "objectHandle = " << pCallbackData->pObjects[i].objectHandle << "\n";
            if (pCallbackData->pObjects[i].pObjectName) {
                message << std::string("\t\t\t") << "objectName   = <" << pCallbackData->pObjects[i].pObjectName
                        << ">\n";
            }
        }
    }

    std::string str = message.str();

    switch (messageSeverity) {
    case vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose:
        [[fallthrough]];
    case vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo:
        [[fallthrough]];
    case vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning: {
        std::cout << str << std::endl;
        break;
    }
    case vk::DebugUtilsMessageSeverityFlagBitsEXT::eError: {
        std::cerr << str << std::endl;
#if defined(_WIN64)
        MessageBox(NULL, str.c_str(), "VULKAN ERROR!!!", MB_ICONERROR | MB_OK);
#endif
        break;
    }
    }

    return false;
}

VulkanRenderingDevice::VulkanRenderingDevice(const VulkanRenderingWindow* window, bool enableValidationLayers) {
    vk::ApplicationInfo applicationInfo;
    applicationInfo.setApiVersion(VK_API_VERSION_1_4);

    std::vector<const char*> enabledInstanceLayers =
        enableValidationLayers ? std::vector<const char*>{"VK_LAYER_KHRONOS_validation"} : std::vector<const char*>{};

    std::vector<const char*> enabledInstanceExtensions = window->getRequiredInstanceExtensions();
    if (enableValidationLayers) {
        enabledInstanceExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }

    vk::InstanceCreateInfo instanceCreateInfo;
    instanceCreateInfo.setPApplicationInfo(&applicationInfo);
    instanceCreateInfo.setPEnabledLayerNames(enabledInstanceLayers);
    instanceCreateInfo.setPEnabledExtensionNames(enabledInstanceExtensions);
    m_instance = vk::createInstanceUnique(instanceCreateInfo);

    if (enableValidationLayers) {
        pfnVkCreateDebugUtilsMessengerEXT = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(
            m_instance->getProcAddr("vkCreateDebugUtilsMessengerEXT"));
        pfnVkDestroyDebugUtilsMessengerEXT = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(
            m_instance->getProcAddr("vkDestroyDebugUtilsMessengerEXT"));

        vk::DebugUtilsMessengerCreateInfoEXT debugUtilsMessengerCreateInfo;
        debugUtilsMessengerCreateInfo.setMessageSeverity(
            vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose | vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo |
            vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning | vk::DebugUtilsMessageSeverityFlagBitsEXT::eError);
        debugUtilsMessengerCreateInfo.setMessageType(vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral |
                                                     vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance |
                                                     vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation);
        debugUtilsMessengerCreateInfo.setPfnUserCallback(&vulkanDebugMessengerCallback);
        m_debugMessenger = m_instance->createDebugUtilsMessengerEXTUnique(debugUtilsMessengerCreateInfo);
    }

    for (const auto& physicalDevice : m_instance->enumeratePhysicalDevices()) {
        const auto& queueFamilyProperties = physicalDevice.getQueueFamilyProperties();

        auto it = std::find_if(queueFamilyProperties.begin(),
                               queueFamilyProperties.end(),
                               [](const vk::QueueFamilyProperties& properties) {
                                   return properties.queueFlags & vk::QueueFlagBits::eGraphics;
                               });

        auto graphicsQueueFamilyIndex = (uint32_t)std::distance(queueFamilyProperties.begin(), it);
        if (graphicsQueueFamilyIndex < queueFamilyProperties.size()) {
            m_graphicsQueueFamilyIndex = graphicsQueueFamilyIndex;
            m_physicalDevice = physicalDevice;
            break;
        }
    }

    const std::vector<const char*> enabledDeviceExtensions = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
    };

    constexpr float queuePriority = 1.f;

    vk::DeviceQueueCreateInfo deviceQueueCreateInfo;
    deviceQueueCreateInfo.setQueueFamilyIndex(m_graphicsQueueFamilyIndex);
    deviceQueueCreateInfo.setQueuePriorities(queuePriority);
    deviceQueueCreateInfo.setQueueCount(1);

    vk::PhysicalDeviceVulkan13Features features13;
    features13.setDynamicRendering(vk::True);
    features13.setSynchronization2(vk::True);

    vk::PhysicalDeviceVulkan12Features features12;
    features12.setBufferDeviceAddress(vk::True);
    features12.setDescriptorIndexing(vk::True);

    vk::PhysicalDeviceFeatures2 features2;

    vk::DeviceCreateInfo deviceCreateInfo;
    deviceCreateInfo.setQueueCreateInfos(deviceQueueCreateInfo);
    deviceCreateInfo.setPEnabledExtensionNames(enabledDeviceExtensions);
    deviceCreateInfo.setPNext(&features2);

    vk::StructureChain<vk::DeviceCreateInfo,
                       vk::PhysicalDeviceFeatures2,
                       vk::PhysicalDeviceVulkan12Features,
                       vk::PhysicalDeviceVulkan13Features>
        structChain{deviceCreateInfo, features2, features12, features13};

    m_logicalDevice = m_physicalDevice.createDeviceUnique(structChain.get<vk::DeviceCreateInfo>());
    m_graphicsQueue = m_logicalDevice->getQueue(m_graphicsQueueFamilyIndex, 0);

    m_surface = vk::UniqueSurfaceKHR{window->createSurface(m_instance.get()), m_instance.get()};

    m_presentMode = vk::PresentModeKHR::eFifo;

    const auto& surfaceFormats = m_physicalDevice.getSurfaceFormatsKHR(m_surface.get());
    auto it = std::find_if(surfaceFormats.begin(), surfaceFormats.end(), [](const vk::SurfaceFormatKHR& surfaceFormat) {
        return surfaceFormat.format == vk::Format::eR8G8B8A8Unorm &&
               surfaceFormat.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear;
    });

    const size_t imageFormatIndex = std::distance(surfaceFormats.begin(), it);
    m_imageFormat = imageFormatIndex < surfaceFormats.size() ? surfaceFormats[imageFormatIndex].format
                    : !surfaceFormats.empty()                ? surfaceFormats[0].format
                                                             : vk::Format::eUndefined;

    const auto& surfaceCapabilities = m_physicalDevice.getSurfaceCapabilitiesKHR(m_surface.get());

    vk::SurfaceTransformFlagBitsKHR preTransform =
        (surfaceCapabilities.supportedTransforms & vk::SurfaceTransformFlagBitsKHR::eIdentity)
            ? vk::SurfaceTransformFlagBitsKHR::eIdentity
            : surfaceCapabilities.currentTransform;

    vk::CompositeAlphaFlagBitsKHR compositeAlpha =
        (surfaceCapabilities.supportedCompositeAlpha & vk::CompositeAlphaFlagBitsKHR::ePreMultiplied)
            ? vk::CompositeAlphaFlagBitsKHR::ePreMultiplied
        : (surfaceCapabilities.supportedCompositeAlpha & vk::CompositeAlphaFlagBitsKHR::ePostMultiplied)
            ? vk::CompositeAlphaFlagBitsKHR::ePostMultiplied
        : (surfaceCapabilities.supportedCompositeAlpha & vk::CompositeAlphaFlagBitsKHR::eInherit)
            ? vk::CompositeAlphaFlagBitsKHR::eInherit
            : vk::CompositeAlphaFlagBitsKHR::eOpaque;

    if (surfaceCapabilities.currentExtent.width == (std::numeric_limits<uint32_t>::max)()) {
        // If the surface size is undefined, the size is set to the size of the
        // images requested.
        m_imageExtent.width = std::clamp((uint32_t)window->getWidth(),
                                         surfaceCapabilities.minImageExtent.width,
                                         surfaceCapabilities.maxImageExtent.width);
        m_imageExtent.height = std::clamp((uint32_t)window->getWidth(),
                                          surfaceCapabilities.minImageExtent.height,
                                          surfaceCapabilities.maxImageExtent.height);
    } else {
        // If the surface size is defined, the swap chain size must match
        m_imageExtent = surfaceCapabilities.currentExtent;
    }

    uint32_t minImageCount = (std::max)(3u, surfaceCapabilities.minImageCount);
    // Some drivers report maxImageCount as 0, so only clamp to max if it is
    // valid.
    if (surfaceCapabilities.maxImageCount > 0) {
        minImageCount = (std::min)(minImageCount, surfaceCapabilities.maxImageCount);
    }

    vk::SwapchainCreateInfoKHR swapchainCreateInfo;
    swapchainCreateInfo.setImageExtent(m_imageExtent);
    swapchainCreateInfo.setImageFormat(m_imageFormat);
    swapchainCreateInfo.setCompositeAlpha(compositeAlpha);
    swapchainCreateInfo.setImageColorSpace(surfaceFormats[imageFormatIndex].colorSpace);
    swapchainCreateInfo.setPreTransform(preTransform);
    swapchainCreateInfo.setPresentMode(m_presentMode);
    swapchainCreateInfo.setMinImageCount(minImageCount);
    swapchainCreateInfo.setSurface(m_surface.get());
    swapchainCreateInfo.setImageSharingMode(vk::SharingMode::eExclusive);
    swapchainCreateInfo.setImageUsage(vk::ImageUsageFlagBits::eStorage | vk::ImageUsageFlagBits::eTransferDst);
    swapchainCreateInfo.setImageArrayLayers(1);
    m_swapchain = m_logicalDevice->createSwapchainKHRUnique(swapchainCreateInfo);

    m_images = m_logicalDevice->getSwapchainImagesKHR(m_swapchain.get());

    vk::ImageViewCreateInfo imageViewCreateInfo(
        {}, {}, vk::ImageViewType::e2D, m_imageFormat, {}, {vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1});

    m_imageViews.reserve(m_images.size());
    for (const auto& image : m_images) {
        imageViewCreateInfo.setImage(image);
        m_imageViews.push_back(m_logicalDevice->createImageViewUnique(imageViewCreateInfo));
    }

    vk::CommandPoolCreateInfo commandPoolCreateInfo;
    commandPoolCreateInfo.setQueueFamilyIndex(m_graphicsQueueFamilyIndex);
    commandPoolCreateInfo.setFlags(vk::CommandPoolCreateFlagBits::eResetCommandBuffer);
    m_commandPool = m_logicalDevice->createCommandPoolUnique(commandPoolCreateInfo);

    vk::CommandBufferAllocateInfo commandBufferAllocateInfo;
    commandBufferAllocateInfo.setLevel(vk::CommandBufferLevel::ePrimary);
    commandBufferAllocateInfo.setCommandPool(m_commandPool.get());
    commandBufferAllocateInfo.setCommandBufferCount(1);
    m_commandBuffer = std::move(m_logicalDevice->allocateCommandBuffersUnique(commandBufferAllocateInfo)[0]);

    m_imageAvailableSemaphore = m_logicalDevice->createSemaphoreUnique({});
    m_renderFinishedSemaphores.resize(m_images.size());
    for (int i = 0; i < m_images.size(); ++i) {
        m_renderFinishedSemaphores[i] = m_logicalDevice->createSemaphoreUnique({});
    }

    vk::FenceCreateInfo fenceCreateInfo;
    fenceCreateInfo.setFlags(vk::FenceCreateFlagBits::eSignaled);
    m_inFlightFence = m_logicalDevice->createFenceUnique(fenceCreateInfo);
}

void VulkanRenderingDevice::beginRendering() {
    [[maybe_unused]] auto result =
        m_logicalDevice->waitForFences(m_inFlightFence.get(), vk::True, std::numeric_limits<uint64_t>::max());

    m_logicalDevice->resetFences(m_inFlightFence.get());

    const auto [imageIndexResult, imageIndex] = m_logicalDevice->acquireNextImageKHR(
        m_swapchain.get(), std::numeric_limits<uint64_t>::max(), m_imageAvailableSemaphore.get(), nullptr);
    m_imageIndex = imageIndex;

    vk::CommandBufferBeginInfo commandBufferBeginInfo;
    commandBufferBeginInfo.setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
    m_commandBuffer->begin(commandBufferBeginInfo);

    transitionImageLayout(m_commandBuffer.get(),
                          m_images[m_imageIndex],
                          m_imageFormat,
                          vk::ImageLayout::eUndefined,
                          vk::ImageLayout::eGeneral);
}

void VulkanRenderingDevice::endRendering() {
    transitionImageLayout(m_commandBuffer.get(),
                          m_images[m_imageIndex],
                          m_imageFormat,
                          vk::ImageLayout::eGeneral,
                          vk::ImageLayout::ePresentSrcKHR);

    m_commandBuffer->end();

    vk::SemaphoreSubmitInfo waitSemaphoreSubmitInfo;
    waitSemaphoreSubmitInfo.setSemaphore(m_imageAvailableSemaphore.get());
    waitSemaphoreSubmitInfo.setStageMask(vk::PipelineStageFlagBits2::eColorAttachmentOutput);
    waitSemaphoreSubmitInfo.setDeviceIndex(0);
    waitSemaphoreSubmitInfo.setValue(1);

    vk::SemaphoreSubmitInfo signalSemaphoreSubmitInfo;
    signalSemaphoreSubmitInfo.setSemaphore(m_renderFinishedSemaphores[m_imageIndex].get());
    signalSemaphoreSubmitInfo.setStageMask(vk::PipelineStageFlagBits2::eAllGraphics);
    signalSemaphoreSubmitInfo.setDeviceIndex(0);
    signalSemaphoreSubmitInfo.setValue(1);

    vk::CommandBufferSubmitInfo commandBufferSubmitInfo;
    commandBufferSubmitInfo.setCommandBuffer(m_commandBuffer.get());
    commandBufferSubmitInfo.setDeviceMask(0);

    vk::SubmitInfo2 submitInfo;
    submitInfo.setCommandBufferInfos(commandBufferSubmitInfo);
    submitInfo.setWaitSemaphoreInfos(waitSemaphoreSubmitInfo);
    submitInfo.setSignalSemaphoreInfos(signalSemaphoreSubmitInfo);

    m_graphicsQueue.submit2(submitInfo, m_inFlightFence.get());

    vk::SwapchainKHR swapchain = m_swapchain.get();

    vk::PresentInfoKHR presentInfo;
    presentInfo.setSwapchains(swapchain);
    presentInfo.setImageIndices(m_imageIndex);
    presentInfo.setWaitSemaphores(signalSemaphoreSubmitInfo.semaphore);

    [[maybe_unused]] auto result = m_graphicsQueue.presentKHR(presentInfo);
}

void VulkanRenderingDevice::clear(const RenderingColor& color) {
    vk::ClearColorValue clearColor(color);

    vk::ImageSubresourceRange imageSubresourceRange;
    imageSubresourceRange.setAspectMask(vk::ImageAspectFlagBits::eColor);
    imageSubresourceRange.setBaseMipLevel(0);
    imageSubresourceRange.setLevelCount(VK_REMAINING_MIP_LEVELS);
    imageSubresourceRange.setBaseArrayLayer(0);
    imageSubresourceRange.setLayerCount(VK_REMAINING_ARRAY_LAYERS);

    m_commandBuffer->clearColorImage(
        m_images[m_imageIndex], vk::ImageLayout::eGeneral, clearColor, imageSubresourceRange);
}

void VulkanRenderingDevice::bind(RenderingPipeline* pipeline) {
    VulkanRenderingPipeline* vulkanPipeline = static_cast<VulkanRenderingPipeline*>(pipeline);

    vk::DescriptorImageInfo descriptorImageInfo;
    descriptorImageInfo.setImageView(m_imageViews[m_imageIndex].get());
    descriptorImageInfo.setImageLayout(vk::ImageLayout::eGeneral);

    vk::WriteDescriptorSet drawImageWrite;
    drawImageWrite.setDstBinding(0);
    drawImageWrite.setDstSet(vulkanPipeline->getDescriptorSet());
    drawImageWrite.setDescriptorCount(1);
    drawImageWrite.setDescriptorType(vk::DescriptorType::eStorageImage);
    drawImageWrite.setImageInfo(descriptorImageInfo);

    m_logicalDevice->updateDescriptorSets({drawImageWrite}, {});

    float seconds = (float)glfwGetTime();

    m_commandBuffer->pushConstants(
        vulkanPipeline->getPipelineLayout(), vk::ShaderStageFlagBits::eCompute, 0, sizeof(float), &seconds);

    m_commandBuffer->bindPipeline(vk::PipelineBindPoint::eCompute, vulkanPipeline->getPipeline());

    m_commandBuffer->bindDescriptorSets(vk::PipelineBindPoint::eCompute,
                                        vulkanPipeline->getPipelineLayout(),
                                        0,
                                        {vulkanPipeline->getDescriptorSet()},
                                        {});
}

void VulkanRenderingDevice::dispatch() {
    m_commandBuffer->dispatch(
        (uint32_t)std::ceil(m_imageExtent.width / 16.0), (uint32_t)std::ceil(m_imageExtent.height / 16.0), 1);
}

void VulkanRenderingDevice::waitIdle() {
    m_logicalDevice->waitIdle();
}

std::unique_ptr<RenderingPipeline>
VulkanRenderingDevice::createRenderingPipeline(const std::string& shaderFileName) const {
    return std::make_unique<VulkanRenderingPipeline>(this, shaderFileName);
}

vk::PipelineStageFlags2KHR VulkanRenderingDevice::getStageFlagsForLayout(vk::ImageLayout layout) {
    switch (layout) {
    case vk::ImageLayout::eUndefined:
        return vk::PipelineStageFlagBits2KHR::eTopOfPipe;
    case vk::ImageLayout::eTransferDstOptimal:
    case vk::ImageLayout::eTransferSrcOptimal:
        return vk::PipelineStageFlagBits2KHR::eTransfer;
    case vk::ImageLayout::eColorAttachmentOptimal:
        return vk::PipelineStageFlagBits2KHR::eColorAttachmentOutput;
    case vk::ImageLayout::eDepthStencilAttachmentOptimal:
        return vk::PipelineStageFlagBits2KHR::eEarlyFragmentTests | vk::PipelineStageFlagBits2KHR::eLateFragmentTests;
    case vk::ImageLayout::eShaderReadOnlyOptimal:
        return vk::PipelineStageFlagBits2KHR::eFragmentShader | vk::PipelineStageFlagBits2KHR::eComputeShader;
    case vk::ImageLayout::ePresentSrcKHR:
        return vk::PipelineStageFlagBits2KHR::eBottomOfPipe; // Handled by presentation engine
    default:
        return vk::PipelineStageFlagBits2KHR::eTopOfPipe; // Fallback or throw
                                                          // error
    }
}

vk::AccessFlags2KHR VulkanRenderingDevice::getAccessFlagsForLayout(vk::ImageLayout layout) {
    switch (layout) {
    case vk::ImageLayout::eUndefined:
        return vk::AccessFlagBits2KHR::eNone;
    case vk::ImageLayout::eTransferDstOptimal:
        return vk::AccessFlagBits2KHR::eTransferWrite;
    case vk::ImageLayout::eTransferSrcOptimal:
        return vk::AccessFlagBits2KHR::eTransferRead;
    case vk::ImageLayout::eColorAttachmentOptimal:
        return vk::AccessFlagBits2KHR::eColorAttachmentWrite;
    case vk::ImageLayout::eDepthStencilAttachmentOptimal:
        return vk::AccessFlagBits2KHR::eDepthStencilAttachmentWrite;
    case vk::ImageLayout::eShaderReadOnlyOptimal:
        return vk::AccessFlagBits2KHR::eShaderRead;
    case vk::ImageLayout::ePresentSrcKHR:
        return vk::AccessFlagBits2KHR::eNone;
    default:
        return vk::AccessFlagBits2KHR::eNone;
    }
}

void VulkanRenderingDevice::transitionImageLayout(vk::CommandBuffer commandBuffer,
                                                  vk::Image image,
                                                  vk::Format format,
                                                  vk::ImageLayout oldLayout,
                                                  vk::ImageLayout newLayout,
                                                  uint32_t mipLevels,
                                                  uint32_t arrayLayers) {
    // Determine the stage and access masks using the new Synchronization 2
    // flags
    vk::PipelineStageFlags2KHR srcStage = getStageFlagsForLayout(oldLayout);
    vk::PipelineStageFlags2KHR dstStage = getStageFlagsForLayout(newLayout);
    vk::AccessFlags2KHR srcAccess = getAccessFlagsForLayout(oldLayout);
    vk::AccessFlags2KHR dstAccess = getAccessFlagsForLayout(newLayout);

    vk::ImageMemoryBarrier2KHR imageMemoryBarrier(
        srcStage,  // srcStageMask
        srcAccess, // srcAccessMask
        dstStage,  // dstStageMask
        dstAccess, // dstAccessMask
        oldLayout,
        newLayout,
        VK_QUEUE_FAMILY_IGNORED, // Use VK_QUEUE_FAMILY_IGNORED for layout
                                 // transition within the same queue
        VK_QUEUE_FAMILY_IGNORED,
        image,
        vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, // Adjust aspect mask based on
                                                                   // format (color/depth/stencil)
                                  0,
                                  mipLevels,
                                  0,
                                  arrayLayers));

    // If using depth/stencil formats, adjust the aspect mask
    if (format == vk::Format::eD32Sfloat || format == vk::Format::eD32SfloatS8Uint ||
        format == vk::Format::eD24UnormS8Uint) {
        imageMemoryBarrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eDepth;
    }

    // TODO: Add logic for stencil

    vk::DependencyInfoKHR dependencyInfo(vk::DependencyFlagBits::eByRegion, // Optional flags
                                         nullptr,           // Buffer memory barriers (none for this transition)
                                         nullptr,           // Memory barriers (none for this transition)
                                         imageMemoryBarrier // Image memory barriers
    );

    // Record the barrier using pipelineBarrier2
    commandBuffer.pipelineBarrier2(dependencyInfo);
}

} // namespace broken
