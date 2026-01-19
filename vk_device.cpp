#include "vk_device.hpp"
#include "log.hpp"

#if defined(_WIN64)
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#endif
#include <GLFW/glfw3.h>

#include <sstream>

#if defined(_DEBUG)
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
#endif

namespace broken::vulkan {

#if defined(_DEBUG)
[[maybe_unused]] static VKAPI_ATTR vk::Bool32
    VKAPI_CALL vulkan_debug_messenger_callback(vk::DebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                                               vk::DebugUtilsMessageTypeFlagsEXT messageTypes,
                                               vk::DebugUtilsMessengerCallbackDataEXT const* pCallbackData,
                                               void* /*pUserData*/) {
    std::ostringstream message;

    message << vk::to_string(messageTypes) << ":\n";
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
        break;
    case vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo:
        break;
    case vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning:
        break;
    case vk::DebugUtilsMessageSeverityFlagBitsEXT::eError:
        break;
    }

    if (messageSeverity == vk::DebugUtilsMessageSeverityFlagBitsEXT::eError) {
        broken_error("{}", str);
#if defined(_WIN64)
        MessageBox(NULL, str.c_str(), "Vulkan Error!", MB_ICONERROR | MB_OK);
        exit(EXIT_FAILURE);
#endif
    } else {
        broken_debug("{}", str);
    }

    return false;
}
#endif

[[maybe_unused]] static std::vector<const char*> get_required_instance_layers() noexcept {
    std::vector<const char*> layers;
#if defined(_DEBUG)
    layers.push_back("VK_LAYER_KHRONOS_validation");
#endif
    return layers;
}

[[maybe_unused]] static std::vector<const char*> get_required_instance_extensions() noexcept {
    uint32_t count = 0;
    const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&count);
    std::vector<const char*> extensions(glfwExtensions, glfwExtensions + count);
#if defined(_DEBUG)
    extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
#endif
    return extensions;
}

[[maybe_unused]] static std::vector<const char*> get_required_device_extensions() noexcept {
    return {VK_KHR_SWAPCHAIN_EXTENSION_NAME};
}

device::device(GLFWwindow* window) noexcept {
    int width = 0, height = 0;
    glfwGetFramebufferSize(window, &width, &height);

    vk::ApplicationInfo applicationInfo;
    applicationInfo.setApiVersion(VK_API_VERSION_1_4);

    std::vector<const char*> enabledInstanceLayers = get_required_instance_layers();
    std::vector<const char*> enabledInstanceExtensions = get_required_instance_extensions();

    vk::InstanceCreateInfo instanceCreateInfo;
    instanceCreateInfo.setPApplicationInfo(&applicationInfo);
    instanceCreateInfo.setPEnabledLayerNames(enabledInstanceLayers);
    instanceCreateInfo.setPEnabledExtensionNames(enabledInstanceExtensions);
    instance = vk::createInstanceUnique(instanceCreateInfo);

#if defined(_DEBUG)
    pfnVkCreateDebugUtilsMessengerEXT =
        reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(instance->getProcAddr("vkCreateDebugUtilsMessengerEXT"));
    if (!pfnVkCreateDebugUtilsMessengerEXT)
        broken_error("Failed to load 'vkCreateDebugUtilsMessengerEXT' from Vulkan instance!!!");
    pfnVkDestroyDebugUtilsMessengerEXT =
        reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(instance->getProcAddr("vkDestroyDebugUtilsMessengerEXT"));
    if (!pfnVkCreateDebugUtilsMessengerEXT)
        broken_error("Failed to load 'vkDestroyDebugUtilsMessengerEXT' from Vulkan instance!!!");

    vk::DebugUtilsMessengerCreateInfoEXT debugUtilsMessengerCreateInfo;
    debugUtilsMessengerCreateInfo.setMessageSeverity(
        vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose | vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo |
        vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning | vk::DebugUtilsMessageSeverityFlagBitsEXT::eError);
    debugUtilsMessengerCreateInfo.setMessageType(vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral |
                                                 vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance |
                                                 vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation);
    debugUtilsMessengerCreateInfo.setPfnUserCallback(&vulkan_debug_messenger_callback);
    debugMessenger = instance->createDebugUtilsMessengerEXTUnique(debugUtilsMessengerCreateInfo);
#endif

    for (const auto& candidatePhysicalDevice : instance->enumeratePhysicalDevices()) {
        const auto& queueFamilyProperties = candidatePhysicalDevice.getQueueFamilyProperties();

        auto it = std::find_if(queueFamilyProperties.begin(),
                               queueFamilyProperties.end(),
                               [](const vk::QueueFamilyProperties& properties) {
                                   return properties.queueFlags & vk::QueueFlagBits::eGraphics;
                               });

        auto graphicsQueueIndex = (uint32_t)std::distance(queueFamilyProperties.begin(), it);
        if (graphicsQueueIndex < queueFamilyProperties.size()) {
            graphicsQueueFamilyIndex = graphicsQueueIndex;
            physicalDevice = candidatePhysicalDevice;
            break;
        }
    }

    std::vector<const char*> enabledDeviceExtensions = get_required_device_extensions();

    constexpr float queuePriority = 1.f;

    vk::DeviceQueueCreateInfo deviceQueueCreateInfo;
    deviceQueueCreateInfo.setQueueFamilyIndex(graphicsQueueFamilyIndex);
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

    logicalDevice = physicalDevice.createDeviceUnique(structChain.get<vk::DeviceCreateInfo>());
    graphicsQueue = logicalDevice->getQueue(graphicsQueueFamilyIndex, 0);

    VkSurfaceKHR rawSurface = VK_NULL_HANDLE;
    glfwCreateWindowSurface(instance.get(), window, nullptr, &rawSurface);
    surface = vk::UniqueSurfaceKHR(rawSurface, instance.get());

    recreate_swapchain({(uint32_t)width, (uint32_t)height}, {});

    create_color_buffer(swapchainExtent);
    create_depth_buffer(swapchainExtent);
}

void device::recreate_swapchain(const vk::Extent2D& extent, vk::UniqueSwapchainKHR oldSwapchain) {
    const auto presentMode = vk::PresentModeKHR::eFifo;

    const auto& surfaceFormats = physicalDevice.getSurfaceFormatsKHR(surface.get());
    auto it = std::find_if(surfaceFormats.begin(), surfaceFormats.end(), [](const vk::SurfaceFormatKHR& surfaceFormat) {
        return surfaceFormat.format == vk::Format::eR8G8B8A8Unorm &&
               surfaceFormat.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear;
    });

    const size_t imageFormatIndex = std::distance(surfaceFormats.begin(), it);
    swapchainFormat = imageFormatIndex < surfaceFormats.size() ? surfaceFormats[imageFormatIndex].format
                      : !surfaceFormats.empty()                ? surfaceFormats[0].format
                                                               : vk::Format::eUndefined;

    const auto& surfaceCapabilities = physicalDevice.getSurfaceCapabilitiesKHR(surface.get());

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
        swapchainExtent.width = std::clamp(
            extent.width, surfaceCapabilities.minImageExtent.width, surfaceCapabilities.maxImageExtent.width);
        swapchainExtent.height = std::clamp(
            extent.height, surfaceCapabilities.minImageExtent.height, surfaceCapabilities.maxImageExtent.height);
    } else {
        // If the surface size is defined, the swap chain size must match
        swapchainExtent = surfaceCapabilities.currentExtent;
    }

    uint32_t minImageCount = (std::max)(3u, surfaceCapabilities.minImageCount);
    // Some drivers report maxImageCount as 0, so only clamp to max if it is
    // valid.
    if (surfaceCapabilities.maxImageCount > 0) {
        minImageCount = (std::min)(minImageCount, surfaceCapabilities.maxImageCount);
    }

    swapchainExtent = extent;

    vk::SwapchainCreateInfoKHR createInfo(
        {},
        surface.get(),
        minImageCount,
        swapchainFormat,
        vk::ColorSpaceKHR::eSrgbNonlinear,
        swapchainExtent,
        1,
        vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferDst,
        vk::SharingMode::eExclusive,
        0,
        nullptr,
        preTransform,
        compositeAlpha,
        presentMode,
        VK_TRUE,
        oldSwapchain.get());

    swapchain = logicalDevice->createSwapchainKHRUnique(createInfo);
    swapchainImages = logicalDevice->getSwapchainImagesKHR(swapchain.get());

    for (size_t i = 0; i < swapchainImages.size(); i++) {
        vk::ImageViewCreateInfo viewInfo({},
                                         swapchainImages[i],
                                         vk::ImageViewType::e2D,
                                         swapchainFormat,
                                         vk::ComponentMapping(),
                                         vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1));
        swapchainImageViews.push_back(logicalDevice->createImageViewUnique(viewInfo));
    }
}

void device::create_color_buffer(const vk::Extent2D& extent) {
    colorExtent = extent;
    create_image(vk::Extent3D(colorExtent.width, colorExtent.height, 1),
                 colorFormat,
                 vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferSrc |
                     vk::ImageUsageFlagBits::eStorage,
                 vk::MemoryPropertyFlagBits::eDeviceLocal,
                 colorImage,
                 colorImageMemory);

    vk::ImageViewCreateInfo viewInfo = {
        {},
        colorImage.get(),
        vk::ImageViewType::e2D,
        colorFormat,
        vk::ComponentMapping(),
        vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1),
    };
    colorImageView = logicalDevice->createImageViewUnique(viewInfo);
}

void device::create_depth_buffer(const vk::Extent2D& extent) {
    depthExtent = extent;
    create_image(vk::Extent3D(depthExtent.width, depthExtent.height, 1),
                 depthFormat,
                 vk::ImageUsageFlagBits::eDepthStencilAttachment,
                 vk::MemoryPropertyFlagBits::eDeviceLocal,
                 depthImage,
                 depthImageMemory);

    vk::ImageViewCreateInfo viewInfo = {
        {},
        depthImage.get(),
        vk::ImageViewType::e2D,
        depthFormat,
        vk::ComponentMapping(),
        vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eDepth, 0, 1, 0, 1),
    };
    depthImageView = logicalDevice->createImageViewUnique(viewInfo);
}

uint32_t device::find_memory_type(uint32_t typeFilter, vk::MemoryPropertyFlags properties) const noexcept {
    vk::PhysicalDeviceMemoryProperties memProperties = physicalDevice.getMemoryProperties();
    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
        if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties)
            return i;
    }
    return uint32_t(-1);
}

void device::create_buffer(vk::DeviceSize size,
                           vk::BufferUsageFlags usage,
                           vk::MemoryPropertyFlags properties,
                           vk::UniqueBuffer& buffer,
                           vk::UniqueDeviceMemory& memory) const noexcept {
    vk::BufferCreateInfo bufferInfo({}, size, usage, vk::SharingMode::eExclusive);
    buffer = logicalDevice->createBufferUnique(bufferInfo);
    vk::MemoryRequirements memReqs = logicalDevice->getBufferMemoryRequirements(buffer.get());
    vk::MemoryAllocateInfo allocInfo(memReqs.size, find_memory_type(memReqs.memoryTypeBits, properties));
    memory = logicalDevice->allocateMemoryUnique(allocInfo);
    logicalDevice->bindBufferMemory(buffer.get(), memory.get(), 0);
}

void device::create_image(vk::Extent3D extent,
                          vk::Format format,
                          vk::ImageUsageFlags usage,
                          vk::MemoryPropertyFlags properties,
                          vk::UniqueImage& image,
                          vk::UniqueDeviceMemory& memory,
                          vk::ImageTiling tiling,
                          vk::ImageType imageType,
                          uint32_t mipLevels,
                          uint32_t arrayLayers,
                          vk::SampleCountFlagBits samples) const noexcept {
    vk::ImageCreateInfo imageInfo;
    imageInfo.imageType = imageType;
    imageInfo.extent = extent;
    imageInfo.mipLevels = mipLevels;
    imageInfo.arrayLayers = arrayLayers;
    imageInfo.format = format;
    imageInfo.tiling = tiling;
    imageInfo.initialLayout = vk::ImageLayout::eUndefined;
    imageInfo.usage = usage;
    imageInfo.samples = samples;
    imageInfo.sharingMode = vk::SharingMode::eExclusive;

    image = logicalDevice->createImageUnique(imageInfo);

    vk::MemoryRequirements memRequirements = logicalDevice->getImageMemoryRequirements(image.get());

    vk::MemoryAllocateInfo allocInfo;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = find_memory_type(memRequirements.memoryTypeBits, properties);

    memory = logicalDevice->allocateMemoryUnique(allocInfo);

    logicalDevice->bindImageMemory(image.get(), memory.get(), 0);
}

void device::copy_image_to_image(vk::CommandBuffer cmd,
                                 vk::Image src,
                                 vk::Image dst,
                                 vk::Extent2D srcSize,
                                 vk::Extent2D dstSize) const noexcept {
    vk::ImageBlit2 blitRegion;

    blitRegion.srcOffsets[0] = vk::Offset3D{0, 0, 0};
    blitRegion.srcOffsets[1] =
        vk::Offset3D{static_cast<int32_t>(srcSize.width), static_cast<int32_t>(srcSize.height), 1};

    blitRegion.dstOffsets[0] = vk::Offset3D{0, 0, 0};
    blitRegion.dstOffsets[1] =
        vk::Offset3D{static_cast<int32_t>(dstSize.width), static_cast<int32_t>(dstSize.height), 1};

    blitRegion.srcSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
    blitRegion.srcSubresource.baseArrayLayer = 0;
    blitRegion.srcSubresource.layerCount = 1;
    blitRegion.srcSubresource.mipLevel = 0;

    blitRegion.dstSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
    blitRegion.dstSubresource.baseArrayLayer = 0;
    blitRegion.dstSubresource.layerCount = 1;
    blitRegion.dstSubresource.mipLevel = 0;

    vk::BlitImageInfo2 blitInfo;
    blitInfo.dstImage = dst;
    blitInfo.dstImageLayout = vk::ImageLayout::eTransferDstOptimal;
    blitInfo.srcImage = src;
    blitInfo.srcImageLayout = vk::ImageLayout::eTransferSrcOptimal;
    blitInfo.filter = vk::Filter::eLinear;
    blitInfo.regionCount = 1;
    blitInfo.pRegions = &blitRegion;

    cmd.blitImage2(blitInfo);
}

vk::PipelineStageFlags2KHR device::stage_flags_for_layout(vk::ImageLayout layout) const noexcept {
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

vk::AccessFlags2KHR device::access_flags_for_layout(vk::ImageLayout layout) const noexcept {
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

void device::transition_image_layout(vk::CommandBuffer cmd,
                                     vk::Image image,
                                     vk::Format format,
                                     vk::ImageLayout oldLayout,
                                     vk::ImageLayout newLayout,
                                     uint32_t mipLevels,
                                     uint32_t arrayLayers) const noexcept {
    // Determine the stage and access masks using the new Synchronization 2
    // flags
    vk::PipelineStageFlags2KHR srcStage = stage_flags_for_layout(oldLayout);
    vk::PipelineStageFlags2KHR dstStage = stage_flags_for_layout(newLayout);
    vk::AccessFlags2KHR srcAccess = access_flags_for_layout(oldLayout);
    vk::AccessFlags2KHR dstAccess = access_flags_for_layout(newLayout);

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
    cmd.pipelineBarrier2(dependencyInfo);
}

} // namespace broken::vulkan
