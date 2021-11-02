//project verse, 2017-2021
//by jose pazos perez
//all rights reserved uwu

#ifdef USE_VULKAN

#include "r_vulkan_api.h"
#include "r_api.h"

#include "r_shader.h"
#include "r_vertex.h"
#include "r_textures.h"

#include "ftime.h"
#include "config.h"

#include <set>
#include <numeric>
#include <glm/gtc/matrix_transform.hpp>

using namespace Verse;
using namespace Graphics;

namespace {
    const std::vector<VertexData> vertices = {
        {{-1.f, -1.f, -1.f}, {0.701f, 0.839f, 0.976f}, {1.0f, 0.0f}}, //Light
        {{1.f, -1.f, -1.f}, {0.117f, 0.784f, 0.596f}, {0.0f, 0.0f}}, //Teal
        {{1.f, 1.f, -1.f}, {1.000f, 0.815f, 0.019f}, {0.0f, 1.0f}}, //Yellow
        {{-1.f, 1.f, -1.f}, {0.988f, 0.521f, 0.113f}, {1.0f, 1.0f}}, //Orange
        {{-1.f, -1.f, 1.f}, {0.925f, 0.254f, 0.345f}, {1.0f, 0.0f}}, //Red
        {{1.f, -1.f, 1.f}, {0.925f, 0.235f, 0.647f}, {0.0f, 0.0f}}, //Pink
        {{1.f, 1.f, 1.f}, {0.658f, 0.180f, 0.898f}, {0.0f, 1.0f}}, //Purple
        {{-1.f, 1.f, 1.f}, {0.258f, 0.376f, 0.941f}, {1.0f, 1.0f}}, //Blue
    };

    const std::vector<ui16> indices = {
        0, 3, 1, 3, 2, 1,
        1, 2, 5, 2, 6, 5,
        4, 7, 0, 7, 3, 0,
        3, 7, 2, 7, 6, 2,
        4, 0, 5, 0, 1, 5,
        5, 6, 4, 6, 7, 4,
    };

    const std::vector<const char*> validation_layers{
        "VK_LAYER_KHRONOS_validation"
    };

    const std::vector<const char*> required_device_extensions{
        "VK_KHR_portability_subset",
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };
}



//Initialization
//----------------------------------------

void API::configure() {
    
}

Vulkan API::create(WindowData &win) {
    Vulkan vk;
    
    //---Instance---
    vk.instance = VK::createInstance(win);
    vk.debug_callback = VK::createDebug(vk.instance);
    
    //---Surface---
    vk.surface = VK::createSurface(vk.instance, win);
    
    //---Physical device---
    vk.physical_device = VK::selectPhysicalDevice(vk.instance, vk.surface);
    vk.physical_device_features = VK::getPhysicalDeviceFeatures(vk.physical_device);
    
    //---Queues and logical device---
    vk.cmd.queue_indices = VK::getQueueFamilies(vk.surface, vk.physical_device);
    vk.device = VK::createDevice(vk.physical_device, vk.physical_device_features, vk.cmd.queue_indices);
    vk.cmd.queues = VK::getQueues(vk.device, vk.cmd.queue_indices);
    
    //---Swapchain---
    vk.swapchain = VK::createSwapchain(vk.device, vk.physical_device, vk.surface, vk.cmd.queue_indices, win);
    
    VK::createCommandPools(vk, {"draw", "temp"}, {}, {{"temp", VK_COMMAND_POOL_CREATE_TRANSIENT_BIT}});
    
    VK::createRenderPass(vk);
    VK::createDescriptorSetLayout(vk);
    VK::createGraphicsPipeline(vk);
    
    VK::createFramebuffers(vk);
    VK::createVertexBuffer(vk, vertices);
    VK::createIndexBuffer(vk, indices);
    
    //Test image
    VK::createSampler(vk);
    vk.test_image = Texture::load(vk, "res/graphics/texture.png", Texture::TEXTURE_CHANNELS_RGBA);
    
    VK::createUniformBuffers(vk);
    VK::createDescriptorPool(vk);
    VK::createDescriptorSets(vk);
    
    VK::createCommandBuffers(vk);
    
    VK::createSyncObjects(vk);
    
    return vk;
}

//----------------------------------------



//Device
//----------------------------------------

VkInstance VK::createInstance(WindowData &win) {
    log::graphics("");
    
    //---Instance extensions---
    //      Add extra functionality to Vulkan
    ui32 extension_count;
    SDL_Vulkan_GetInstanceExtensions(win.window, &extension_count, nullptr);
    std::vector<const char *> extension_names(extension_count);
    SDL_Vulkan_GetInstanceExtensions(win.window, &extension_count, extension_names.data());
    log::graphics("Vulkan requested instance extensions: %d", extension_count);
    for (const char* ext : extension_names)
        log::graphics(" - %s", ext);
    
    log::graphics("");
    
    //---Validation layers---
    //      Middleware for existing Vulkan functionality
    //      Primarily used for getting detailed error descriptions, in this case with VK_LAYER_KHRONOS_validation
    ui32 validation_layer_count;
    vkEnumerateInstanceLayerProperties(&validation_layer_count, nullptr);
    std::vector<VkLayerProperties> available_validation_layers(validation_layer_count);
    vkEnumerateInstanceLayerProperties(&validation_layer_count, available_validation_layers.data());
    log::graphics("Vulkan supported validation layers: %d", validation_layer_count);
    for (VkLayerProperties layer : available_validation_layers)
        log::graphics(" - %s", layer.layerName);
    
    log::graphics("");
    
    for (const char* val : validation_layers) {
        bool layer_exists = false;
        for (const auto &layer : available_validation_layers) {
            if (str(val) == str(layer.layerName)) {
                layer_exists = true;
                break;
            }
        }
        if (not layer_exists)
            log::error("Attempted to use a validation layer but it is not supported (%s)", val);
    }
    
    //---App info---
    VkApplicationInfo app_info{};
    app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app_info.pApplicationName = Conf::name.c_str();
    app_info.applicationVersion = VK_MAKE_VERSION(Conf::version[0], Conf::version[1], Conf::version[2]);
    app_info.pEngineName = "Fresa";
    app_info.engineVersion = VK_MAKE_VERSION(Conf::version[0], Conf::version[1], Conf::version[2]);
    app_info.apiVersion = VK_API_VERSION_1_1;
    
    //---Instance create info---
    VkInstanceCreateInfo instance_create_info{};
    instance_create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instance_create_info.pApplicationInfo = &app_info;
    instance_create_info.enabledLayerCount = (int)validation_layers.size();
    instance_create_info.ppEnabledLayerNames = validation_layers.data();
    instance_create_info.enabledExtensionCount = (int)extension_names.size();
    instance_create_info.ppEnabledExtensionNames = extension_names.data();
    
    //---Create instance---
    VkInstance instance;
    
    if (vkCreateInstance(&instance_create_info, nullptr, &instance)!= VK_SUCCESS)
        log::error("Fatal error creating a vulkan instance");
    
    return instance;
}

VkSurfaceKHR VK::createSurface(VkInstance &instance, WindowData &win) {
    VkSurfaceKHR surface;
    
    //---Surface---
    //      It is the abstraction of the window created by SDL to something Vulkan can draw onto
    if (not SDL_Vulkan_CreateSurface(win.window, instance, &surface))
        log::error("Fatal error while creating a vulkan surface (from createSurface): %s", SDL_GetError());
    
    return surface;
}

ui16 VK::ratePhysicalDevice(VkSurfaceKHR &surface, VkPhysicalDevice &physical_device) {
    ui16 score = 16;
    
    
    //---Device properties---
    //      Holds information about the GPU, such as if it is discrete or integrated
    VkPhysicalDeviceProperties device_properties;
    vkGetPhysicalDeviceProperties(physical_device, &device_properties);
    
    //: Prefer a discrete GPU
    if (device_properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
        score += 256;
    
    
    //---Features---
    //      What capabilites are supported with this device
    //VkPhysicalDeviceFeatures device_features = getPhysicalDeviceFeatures(physical_device);
    
    //: (optional) Anisotropy
    //      Helps when rendering with perspective
    //      Provides a higher quality at the cost of performance
    //      Not all devices suppoport it, so it has to be checked for and enabled
    //  if (not device_features.samplerAnisotropy) return 0;
    
    
    //---Queues---
    //      Different execution ports of the GPU, command buffers are submitted here
    //      There are different spetialized queue families, such as present and graphics
    QueueIndices queue_indices = getQueueFamilies(surface, physical_device);
    if (queue_indices.compute.has_value())
        score += 16;
    if (not queue_indices.present.has_value())
        return 0;
    if (not queue_indices.graphics.has_value())
        return 0;
    
    
    //---Extensions---
    //      Not everything is core in Vulkan, so we need to enable some extensions
    //      Here we check for required extensions (defined up top) and choose physical devices that support them
    //      The main one is vkSwapchainKHR, the surface we draw onto, which is implementation dependent
    ui32 extension_count;
    vkEnumerateDeviceExtensionProperties(physical_device, nullptr, &extension_count, nullptr);
    std::vector<VkExtensionProperties> available_extensions(extension_count);
    vkEnumerateDeviceExtensionProperties(physical_device, nullptr, &extension_count, available_extensions.data());
    
    std::set<std::string> required_extensions(required_device_extensions.begin(), required_device_extensions.end());
    
    for (VkExtensionProperties extension : available_extensions)
        required_extensions.erase(extension.extensionName);

    if (not required_extensions.empty())
        return 0;
    
    //---Swapchain---
    //      Make sure that the device supports swapchain presentation
    VK::SwapchainSupportData swapchain_support = VK::getSwapchainSupport(surface, physical_device);
    if (swapchain_support.formats.empty() or swapchain_support.present_modes.empty())
        return 0;
    
    return score;
}

VkPhysicalDevice VK::selectPhysicalDevice(VkInstance &instance, VkSurfaceKHR &surface) {
    VkPhysicalDevice physical_device = VK_NULL_HANDLE;
    
    //---Show requested device extensions---
    log::graphics("Vulkan required device extensions: %d", required_device_extensions.size());
    for (const char* ext : required_device_extensions)
        log::graphics(" - %s", ext);
    log::graphics("");
    
    
    //---Get available physical devices---
    ui32 device_count = 0;
    vkEnumeratePhysicalDevices(instance, &device_count, nullptr);
    if (device_count == 0)
        log::error("There are no GPUs with vulkan support!");
    
    std::vector<VkPhysicalDevice> devices(device_count);
    vkEnumeratePhysicalDevices(instance, &device_count, devices.data());
    log::graphics("Vulkan physical devices: %d", device_count);
    
    
    //---Rate physical devices---
    //      Using ratePhysicalDevice(), assign a number to each available GPU
    //      Highest number gets chosen as the main physical device for the program
    //      It is possible to use multiple GPUs, but we won't support it yet
    std::multimap<VkPhysicalDevice, ui16> candidates;
    for (VkPhysicalDevice device : devices)
        candidates.insert(std::make_pair(device, ratePhysicalDevice(surface, device)));
    auto chosen = std::max_element(candidates.begin(), candidates.end(), [](auto &a, auto &b){ return a.second < b.second;});
    if (chosen->second > 0)
        physical_device = chosen->first;
    
    
    //---Show the result of the process---
    for (VkPhysicalDevice device : devices) {
        VkPhysicalDeviceProperties device_properties;
        vkGetPhysicalDeviceProperties(device, &device_properties);
        log::graphics(((device == physical_device) ? " > %s" : " - %s"), device_properties.deviceName);
    }
    log::graphics("");
    
    if (physical_device == VK_NULL_HANDLE)
        log::error("No GPU passed the vulkan physical device requirements.");
    
    return physical_device;
}

VkPhysicalDeviceFeatures VK::getPhysicalDeviceFeatures(VkPhysicalDevice &physical_device) {
    VkPhysicalDeviceFeatures device_features;
    vkGetPhysicalDeviceFeatures(physical_device, &device_features);
    return device_features;
}

VK::QueueIndices VK::getQueueFamilies(VkSurfaceKHR &surface, VkPhysicalDevice &physical_device) {
    //---Queues---
    //      Different execution ports of the GPU, command buffers are submitted here
    //      There are different spetialized queue families, such as present, graphics and compute
    
    
    //---Get all available queues---
    ui32 queue_family_count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_family_count, nullptr);
    std::vector<VkQueueFamilyProperties> queue_family_list(queue_family_count);
    vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_family_count, queue_family_list.data());
    
    
    //---Select desired queues---
    //      Using the QueueIndices struct, which has 3 std::optional indices, for:
    //          - Graphics: pipeline operations, including vertex/fragment shaders and drawing
    //          - Present: send framebuffers to the screen
    //          - Compute: for compute shaders
    //      Not all queues are needed, and in the future more queues can be created for multithread support
    //      Made so present and graphics queue can be the same
    QueueIndices queue_indices;
    for (int i = 0; i < queue_family_list.size(); i++) {
        if (not queue_indices.present.has_value()) {
            VkBool32 present_support = false;
            vkGetPhysicalDeviceSurfaceSupportKHR(physical_device, i, surface, &present_support);
            if(present_support)
                queue_indices.present = i;
        }
        
        if (not queue_indices.graphics.has_value() and (queue_family_list[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)) {
            queue_indices.graphics = i;
            continue;
        }
        
        if (not queue_indices.compute.has_value() and (queue_family_list[i].queueFlags & VK_QUEUE_COMPUTE_BIT)) {
            queue_indices.compute = i;
            continue;
        }
        
        if (queue_indices.graphics.has_value() and queue_indices.present.has_value() and queue_indices.compute.has_value())
            break;
    }
    
    return queue_indices;
}

VkDevice VK::createDevice(VkPhysicalDevice &physical_device, VkPhysicalDeviceFeatures &physical_device_features, QueueIndices &queue_indices) {
    //---Logical device---
    //      Vulkan GPU driver, it encapsulates the physical device
    //      Needs to be passed to almost every Vulkan function
    VkDevice device;
    
    
    //---Create the selected queues---
    std::vector<VkDeviceQueueCreateInfo> queue_create_infos;
    
    std::set<ui32> unique_queue_families{};
    if (queue_indices.graphics.has_value())
        unique_queue_families.insert(queue_indices.graphics.value());
    if (queue_indices.present.has_value())
        unique_queue_families.insert(queue_indices.present.value());
    if (queue_indices.compute.has_value())
        unique_queue_families.insert(queue_indices.compute.value());
    log::graphics("Vulkan queue families: %d", unique_queue_families.size());
    
    int i = 0;
    std::vector<float> priorities{ 1.0f, 1.0f, 0.5f };
    
    for (ui32 family : unique_queue_families) {
        VkDeviceQueueCreateInfo queue_graphics_info{};
        queue_graphics_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queue_graphics_info.queueFamilyIndex = family;
        queue_graphics_info.queueCount = 1;
        queue_graphics_info.pQueuePriorities = &priorities[i];
        queue_create_infos.push_back(queue_graphics_info);
        i++;
    }
    
    
    //---Device required features---
    //      Enable some features here, try to keep it as small as possible
    //: (optional) Anisotropy - vk.physical_device_features.samplerAnisotropy = VK_TRUE;
    
    
    //---Create device---
    VkDeviceCreateInfo device_create_info{};
    device_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    
    //: Add the queues we selected before to be created
    device_create_info.queueCreateInfoCount = (int)queue_create_infos.size();
    device_create_info.pQueueCreateInfos = queue_create_infos.data();
    device_create_info.enabledExtensionCount = (int)required_device_extensions.size();
    device_create_info.ppEnabledExtensionNames = required_device_extensions.data();
    device_create_info.pEnabledFeatures = &physical_device_features;
    
    device_create_info.enabledLayerCount = (int)validation_layers.size();
    device_create_info.ppEnabledLayerNames = validation_layers.data();
    
    if (vkCreateDevice(physical_device, &device_create_info, nullptr, &device)!= VK_SUCCESS)
        log::error("Error creating a vulkan logical device");
    
    return device;
}

VK::QueueData VK::getQueues(VkDevice &device, QueueIndices &queue_indices) {
    VK::QueueData queues;
    
    if (queue_indices.graphics.has_value()) {
        vkGetDeviceQueue(device, queue_indices.graphics.value(), 0, &queues.graphics);
        log::graphics(" - Graphics (%d)", queue_indices.graphics.value());
    }
    if (queue_indices.present.has_value()) {
        vkGetDeviceQueue(device, queue_indices.present.value(), 0, &queues.present);
        log::graphics(" - Present (%d)", queue_indices.present.value());
    }
    if (queue_indices.compute.has_value()) {
        vkGetDeviceQueue(device, queue_indices.compute.value(), 0, &queues.compute);
        log::graphics(" - Compute (%d)", queue_indices.compute.value());
    }
    log::graphics("");
    
    return queues;
}

//----------------------------------------



//Swapchain
//----------------------------------------

VK::SwapchainSupportData VK::getSwapchainSupport(VkSurfaceKHR &surface, VkPhysicalDevice &physical_device) {
    SwapchainSupportData swapchain_support;
    
    //---Surface capabilities---
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physical_device, surface, &swapchain_support.capabilities);
    
    //---Surface formats---
    ui32 format_count;
    vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface, &format_count, nullptr);
    if (format_count != 0) {
        swapchain_support.formats.resize(format_count);
        vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface, &format_count, swapchain_support.formats.data());
    }
    
    //---Surface present modes---
    ui32 present_mode_count;
    vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device, surface, &present_mode_count, nullptr);
    if (present_mode_count != 0) {
        swapchain_support.present_modes.resize(present_mode_count);
        vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device, surface, &present_mode_count, swapchain_support.present_modes.data());
    }
    
    return swapchain_support;
}

VkSurfaceFormatKHR VK::selectSwapSurfaceFormat(SwapchainSupportData &support) {
    //---Surface format---
    //      The internal format for the vulkan surface
    //      It might seem weird to use BGRA instead of RGBA, but displays usually use this pixel data format instead
    //      Vulkan automatically converts our framebuffers to this space so we don't need to worry
    //      We also select a nonlinear SRGB color space to correctly represent images
    //      If all fails, it will still select a format, but it might not be the perfect one
    for (VkSurfaceFormatKHR fmt : support.formats) {
        if (fmt.format == VK_FORMAT_B8G8R8A8_SRGB && fmt.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
            return fmt;
    }
    log::warn("A non ideal format has been selected for the swap surface, since BGRA SRGB is not supported. You might experience that the graphics present in unexpected colors. Please check the GPU support for ideal representation.");
    return support.formats[0];
}

VkPresentModeKHR VK::selectSwapPresentMode(SwapchainSupportData &support) {
    //---Surface present mode---
    //      The way the buffers are swaped to the screen
    //      - Fifo: Vsync, when the queue is full the program waits
    //      - Mailbox: Triple buffering, the program replaces the last images of the queue, less latency but more power consumption
    //      Not all GPUs support mailbox (for example integrated Intel GPUs), so while it is preferred, Fifo can be used as well
    //      Maybe in the future offer the user the opportunity to choose the desired mode
    
    for (VkPresentModeKHR mode : support.present_modes) {
        if (mode == VK_PRESENT_MODE_MAILBOX_KHR) {
            log::graphics("Present mode: Mailbox");
            return VK_PRESENT_MODE_MAILBOX_KHR;
        }
    }
    
    log::graphics("Present mode: Fifo");
    return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D VK::selectSwapExtent(SwapchainSupportData &support, WindowData &win) {
    //---Surface extent---
    //      This is the drawable are on the screen
    //      If the current extent is UINT32_MAX, we should calculate the actual extent using WindowData
    //      and clamp it to the min and max supported extent by the GPU
    if (support.capabilities.currentExtent.width != UINT32_MAX)
        return support.capabilities.currentExtent;
    
    int w, h;
    SDL_Vulkan_GetDrawableSize(win.window, &w, &h);
    
    VkExtent2D actual_extent{ static_cast<ui32>(w), static_cast<ui32>(h) };
    
    std::clamp(actual_extent.width, support.capabilities.minImageExtent.width, support.capabilities.maxImageExtent.width);
    std::clamp(actual_extent.height, support.capabilities.minImageExtent.height, support.capabilities.maxImageExtent.height);
    
    return actual_extent;
}

VkSwapchainData VK::createSwapchain(VkDevice &device, VkPhysicalDevice &physical_device, VkSurfaceKHR &surface,
                                    QueueIndices &queue_indices, WindowData &win) {
    //---Swapchain---
    //      List of images that will get drawn to the screen by the render pipeline
    //      Swapchain data:
    //      - VkFormat format: Image format of the swapchain, usually B8G8R8A8_SRGB
    //      - VkExtent2D extent: The size of the draw area
    //      - VkSwapchainKHR swapchain: Actual swapchain object
    //      - std::vector<VkImage> images: List of swapchain images
    //      - std::vector<VkImageView> image_views: List of their respective image views
    //      - TODO: Depth data
    
    VkSwapchainData swapchain;
    
    
    //---Format, present mode and extent---
    SwapchainSupportData support = getSwapchainSupport(surface, physical_device);
    VkSurfaceFormatKHR surface_format = selectSwapSurfaceFormat(support);
    swapchain.format = surface_format.format;
    VkPresentModeKHR present_mode = selectSwapPresentMode(support);
    swapchain.extent = selectSwapExtent(support, win);
    
    
    //---Number of images---
    ui32 min_image_count = support.capabilities.minImageCount + 1;
    if (support.capabilities.maxImageCount > 0 and min_image_count > support.capabilities.maxImageCount)
        min_image_count = support.capabilities.maxImageCount;
    
    
    //---Create swapchain---
    VkSwapchainCreateInfoKHR create_info{};
    create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    create_info.surface = surface;
    
    //: Swapchain images
    create_info.minImageCount = min_image_count;
    create_info.imageFormat = surface_format.format;
    create_info.imageColorSpace = surface_format.colorSpace;
    create_info.imageExtent = swapchain.extent;
    create_info.imageArrayLayers = 1;
    create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    
    //: Specify the sharing mode for the queues
    //  If the graphics and present are the same queue, it must be concurrent
    std::array<ui32,2> queue_family_indices{ queue_indices.graphics.value(), queue_indices.present.value() };
    if (queue_family_indices[0] != queue_family_indices[1]) {
        create_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        create_info.queueFamilyIndexCount = 2;
        create_info.pQueueFamilyIndices = queue_family_indices.data();
    } else {
        create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    }
    
    //: Other properties
    create_info.preTransform = support.capabilities.currentTransform;
    create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    create_info.presentMode = present_mode;
    create_info.clipped = VK_TRUE;
    
    //: Swapchain recreation, disabled right now
    create_info.oldSwapchain = VK_NULL_HANDLE;
    
    if (vkCreateSwapchainKHR(device, &create_info, nullptr, &swapchain.swapchain)!= VK_SUCCESS)
        log::error("Error creating a vulkan swapchain");
    
    
    //---Swapchain images---
    vkGetSwapchainImagesKHR(device, swapchain.swapchain, &swapchain.size, nullptr);
    swapchain.images.resize(swapchain.size);
    vkGetSwapchainImagesKHR(device, swapchain.swapchain, &swapchain.size, swapchain.images.data());
    
    
    //---Swapchain image views---
    swapchain.image_views.resize(swapchain.size);
    for (int i = 0; i < swapchain.images.size(); i++)
        swapchain.image_views[i] = createImageView(device, swapchain.images[i], VK_IMAGE_ASPECT_COLOR_BIT, swapchain.format);
    
    
    log::graphics("Created a vulkan swapchain");
    return swapchain;
}

void VK::recreateSwapchain(Vulkan &vk, WindowData &win) {
    //TODO: Refactor
    vkDeviceWaitIdle(vk.device);
    
    cleanSwapchain(vk);
    
    vk.swapchain = createSwapchain(vk.device, vk.physical_device, vk.surface, vk.cmd.queue_indices, win);
    
    createRenderPass(vk);
    createGraphicsPipeline(vk);
    
    createFramebuffers(vk);
    
    createUniformBuffers(vk);
    createDescriptorPool(vk);
    createDescriptorSets(vk);
    
    createCommandBuffers(vk);
}

VkFormat VK::chooseSupportedFormat(VkPhysicalDevice &physical_device, const std::vector<VkFormat> &candidates,
                                   VkImageTiling tiling, VkFormatFeatureFlags features) {
    //---Choose supported format---
    //      From a list of candidate formats in order of preference, select a supported VkFormat
    
    for (VkFormat format : candidates) {
        VkFormatProperties props;
        vkGetPhysicalDeviceFormatProperties(physical_device, format, &props);
        
        if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features) {
            return format;
        }
        
        if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features) {
            return format;
        }
    }
    
    log::error("Failed to find a suitable supported format");
    return candidates[0];
}

VkFormat VK::getDepthFormat(Vulkan &vk) {
    //---Choose depth format---
    //      Get the most appropiate format for the depth images of the framebuffer
    return chooseSupportedFormat(vk.physical_device, {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT},
                                 VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
}

void VK::createDepthResources(Vulkan &vk) {
    //---Depth resources---
    //PENDING
}

//----------------------------------------



//Command Pools
//----------------------------------------

void VK::createCommandPools(Vulkan &vk, std::vector<str> keys, std::map<str, ui32> queues, std::map<str, VkCommandPoolCreateFlagBits> flags) {
    for (auto key: keys) {
        VkCommandPoolCreateInfo create_info{};
        create_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        
        create_info.queueFamilyIndex = [queues, vk, key](){
            if (queues.count(key) > 0)
                return queues.at(key);
            return vk.cmd.queue_indices.graphics.value();
        }();
        
        create_info.flags = [flags, key](){
            if (flags.count(key) > 0)
                return flags.at(key);
            return (VkCommandPoolCreateFlagBits)0;
        }();
        
        if (vkCreateCommandPool(vk.device, &create_info, nullptr, &vk.cmd.command_pools[key]) != VK_SUCCESS)
            log::error("Failed to create a vulkan command pool (%s)", key.c_str());
    }
    
    log::graphics("Created all vulkan command pools");
}

//----------------------------------------



//Render Pass
//----------------------------------------

VkSubpassDescription VK::createRenderSubpass() {
    VkAttachmentReference color_attachment_ref{};
    color_attachment_ref.attachment = 0;
    color_attachment_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    
    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &color_attachment_ref;
    
    return subpass;
}

void VK::createRenderPass(Vulkan &vk) {
    VkAttachmentDescription color_attachment{};
    
    color_attachment.format = vk.swapchain.format;
    color_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
    
    color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    
    color_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    color_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    
    color_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    color_attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    
    
    VkSubpassDescription subpass = createRenderSubpass();
    
    VkSubpassDependency dependency{};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask = 0;
    
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    
    
    VkRenderPassCreateInfo create_info{};
    create_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    
    create_info.attachmentCount = 1;
    create_info.pAttachments = &color_attachment;
    
    create_info.subpassCount = 1;
    create_info.pSubpasses = &subpass;
    
    create_info.dependencyCount = 1;
    create_info.pDependencies = &dependency;
    
    if (vkCreateRenderPass(vk.device, &create_info, nullptr, &vk.render_pass) != VK_SUCCESS)
        log::error("Error creating a Vulkan Render Pass");
    
    log::graphics("Created all Vulkan Render Passes");
}

//----------------------------------------



//Pipeline
//----------------------------------------

VK::RenderingCreateInfo VK::prepareRenderInfo(Vulkan &vk) {
    RenderingCreateInfo info;
    
    prepareRenderInfoVertexInput(info); //TODO: Change for actual types and add them here, it will look very nice
    prepareRenderInfoInputAssembly(info);
    prepareRenderInfoViewportState(info, vk.swapchain.extent);
    prepareRenderInfoRasterizer(info);
    prepareRenderInfoMultisampling(info);
    prepareRenderInfoDepthStencil(info);
    prepareRenderInfoColorBlendAttachment(info);
    prepareRenderInfoColorBlendState(info);
    
    return info;
}

void VK::prepareRenderInfoVertexInput(VK::RenderingCreateInfo &info) {
    info.vertex_input = {};
    info.vertex_input.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    
    info.vertex_input_binding_description = Vertex::getBindingDescriptionVK();
    info.vertex_input_attribute_descriptions = Vertex::getAttributeDescriptionsVK<VertexData>();
    
    info.vertex_input.vertexBindingDescriptionCount = 1;
    info.vertex_input.pVertexBindingDescriptions = &info.vertex_input_binding_description;
    info.vertex_input.vertexAttributeDescriptionCount = static_cast<ui32>(info.vertex_input_attribute_descriptions.size());
    info.vertex_input.pVertexAttributeDescriptions = info.vertex_input_attribute_descriptions.data();
}

void VK::prepareRenderInfoInputAssembly(VK::RenderingCreateInfo &info) {
    info.input_assembly = {};
    
    info.input_assembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    info.input_assembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    info.input_assembly.primitiveRestartEnable = VK_FALSE; //Change when using an index buffer
}

void VK::prepareRenderInfoViewportState(VK::RenderingCreateInfo &info, VkExtent2D extent) {
    info.viewport = {};
    info.viewport.x = 0.0f;
    info.viewport.y = 0.0f;
    info.viewport.width = (float)extent.width;
    info.viewport.height = (float)extent.height;
    info.viewport.minDepth = 0.0f;
    info.viewport.maxDepth = 1.0f;
    
    info.scissor = {};
    info.scissor.offset = {0, 0};
    info.scissor.extent = extent;
    
    info.viewport_state = {};
    
    info.viewport_state.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    info.viewport_state.viewportCount = 1;
    info.viewport_state.pViewports = &info.viewport;
    info.viewport_state.scissorCount = 1;
    info.viewport_state.pScissors = &info.scissor;
}

void VK::prepareRenderInfoRasterizer(VK::RenderingCreateInfo &info) {
    info.rasterizer = {};
    
    info.rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    info.rasterizer.depthClampEnable = VK_FALSE;
    info.rasterizer.rasterizerDiscardEnable = VK_FALSE;
    
    info.rasterizer.polygonMode = VK_POLYGON_MODE_FILL; //Fill, Line, Point, requires enabling GPU features
    info.rasterizer.lineWidth = 1.0f; //Larger thickness requires enabling GPU features
    
    info.rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
    info.rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    
    info.rasterizer.depthBiasEnable = VK_FALSE;
    info.rasterizer.depthBiasConstantFactor = 0.0f;
    info.rasterizer.depthBiasClamp = 0.0f;
    info.rasterizer.depthBiasSlopeFactor = 0.0f;
}

void VK::prepareRenderInfoMultisampling(VK::RenderingCreateInfo &info) {
    info.multisampling = {};
    
    info.multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    info.multisampling.sampleShadingEnable = VK_FALSE; //Needs to be enabled in the future
    
    info.multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    info.multisampling.minSampleShading = 1.0f;
    info.multisampling.pSampleMask = nullptr;
    
    info.multisampling.alphaToCoverageEnable = VK_FALSE;
    info.multisampling.alphaToOneEnable = VK_FALSE;
}

void VK::prepareRenderInfoDepthStencil(VK::RenderingCreateInfo &info) {
    info.depth_stencil = {};
    
    info.depth_stencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    
    info.depth_stencil.depthTestEnable = VK_TRUE;
    info.depth_stencil.depthWriteEnable = VK_TRUE;
    
    info.depth_stencil.depthBoundsTestEnable = VK_FALSE;
    info.depth_stencil.stencilTestEnable = VK_FALSE;
    
    info.depth_stencil.depthCompareOp = VK_COMPARE_OP_LESS;
    
    info.depth_stencil.minDepthBounds = 0.0f;
    info.depth_stencil.maxDepthBounds = 1.0f;
    
    info.depth_stencil.front = {};
    info.depth_stencil.back = {};
}

void VK::prepareRenderInfoColorBlendAttachment(VK::RenderingCreateInfo &info) {
    info.color_blend_attachment = {};
    
    info.color_blend_attachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    
    info.color_blend_attachment.blendEnable = VK_TRUE;
    
    info.color_blend_attachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    info.color_blend_attachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    info.color_blend_attachment.colorBlendOp = VK_BLEND_OP_ADD;
    
    info.color_blend_attachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    info.color_blend_attachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    info.color_blend_attachment.alphaBlendOp = VK_BLEND_OP_ADD;
}

void VK::prepareRenderInfoColorBlendState(VK::RenderingCreateInfo &info) {
    info.color_blend_state = {};

    info.color_blend_state.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    
    info.color_blend_state.logicOpEnable = VK_FALSE;
    info.color_blend_state.logicOp = VK_LOGIC_OP_COPY;
    
    info.color_blend_state.attachmentCount = 1;
    info.color_blend_state.pAttachments = &info.color_blend_attachment; //Change for all framebuffers
    
    info.color_blend_state.blendConstants[0] = 0.0f;
    info.color_blend_state.blendConstants[1] = 0.0f;
    info.color_blend_state.blendConstants[2] = 0.0f;
    info.color_blend_state.blendConstants[3] = 0.0f;
}

void VK::createDescriptorSetLayout(Vulkan &vk) {
    std::array<VkDescriptorSetLayoutBinding, 2> layout_binding;
    
    layout_binding[0].binding = 0;
    layout_binding[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    layout_binding[0].descriptorCount = 1;
    layout_binding[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    layout_binding[0].pImmutableSamplers = nullptr;
    
    layout_binding[1].binding = 1;
    layout_binding[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    layout_binding[1].descriptorCount = 1;
    layout_binding[1].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    layout_binding[1].pImmutableSamplers = nullptr;
    
    VkDescriptorSetLayoutCreateInfo create_info{};
    create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    create_info.bindingCount = (ui32)layout_binding.size();
    create_info.pBindings = layout_binding.data();
    
    if (vkCreateDescriptorSetLayout(vk.device, &create_info, nullptr, &vk.descriptor_set_layout) != VK_SUCCESS)
        log::error("Error creating the Vulkan Descriptor Set Layout for Uniform Buffers");
}

void VK::createPipelineLayout(Vulkan &vk) {
    VkPipelineLayoutCreateInfo create_info{};
    create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    
    create_info.setLayoutCount = 1;
    create_info.pSetLayouts = &vk.descriptor_set_layout;
    
    create_info.pushConstantRangeCount = 0;
    create_info.pPushConstantRanges = nullptr;
    
    if (vkCreatePipelineLayout(vk.device, &create_info, nullptr, &vk.pipeline_layout) != VK_SUCCESS)
        log::error("Error creating the Vulkan Pipeline Layout");
    
    log::graphics("Created the Vulkan Pipeline Layout");
}

void VK::createGraphicsPipeline(Vulkan &vk) {
    std::vector<char> vert_shader_code = Graphics::Shader::readSPIRV("res/shaders/test/test.vert.spv");
    std::vector<char> frag_shader_code = Graphics::Shader::readSPIRV("res/shaders/test/test.frag.spv");
    
    Graphics::Shader::ShaderStages stages;
    stages.vert = Graphics::Shader::createShaderModule(vert_shader_code, vk.device);
    stages.frag = Graphics::Shader::createShaderModule(frag_shader_code, vk.device);
    std::vector<VkPipelineShaderStageCreateInfo> stage_info = Graphics::Shader::createShaderStageInfo(stages);
    
    RenderingCreateInfo rendering_create_info = prepareRenderInfo(vk);
    
    VkGraphicsPipelineCreateInfo create_info{};
    create_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    
    create_info.stageCount = (int)stage_info.size();
    create_info.pStages = stage_info.data();
    
    create_info.pVertexInputState = &rendering_create_info.vertex_input;
    create_info.pInputAssemblyState = &rendering_create_info.input_assembly;
    create_info.pViewportState = &rendering_create_info.viewport_state;
    create_info.pRasterizationState = &rendering_create_info.rasterizer;
    create_info.pMultisampleState = &rendering_create_info.multisampling;
    create_info.pDepthStencilState = &rendering_create_info.depth_stencil;
    create_info.pColorBlendState = &rendering_create_info.color_blend_state;
    create_info.pDynamicState = nullptr;
    create_info.pTessellationState = nullptr;
    
    createPipelineLayout(vk);
    
    create_info.layout = vk.pipeline_layout;
    
    create_info.renderPass = vk.render_pass;
    create_info.subpass = 0;
    
    create_info.basePipelineHandle = VK_NULL_HANDLE; //It is not a derived pipeline
    create_info.basePipelineIndex = -1;
    
    if (vkCreateGraphicsPipelines(vk.device, VK_NULL_HANDLE, 1, &create_info, nullptr, &vk.pipeline) != VK_SUCCESS)
        log::error("Error while creating the Vulkan Graphics Pipeline");
    
    log::graphics("Created the Vulkan Graphics Pipeline");
    
    vkDestroyShaderModule(vk.device, stages.vert.value(), nullptr);
    vkDestroyShaderModule(vk.device, stages.frag.value(), nullptr);
}

//----------------------------------------



//Buffers
//----------------------------------------

BufferData VK::createBuffer(Vulkan &vk, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties) {
    BufferData data;
    
    VkBufferCreateInfo create_info{};
    create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    create_info.size = size;
    create_info.usage = usage;
    create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    
    if (vkCreateBuffer(vk.device, &create_info, nullptr, &data.buffer) != VK_SUCCESS)
        log::error("Failed to create a Vulkan Buffer");
    
    VkMemoryRequirements memory_requirements;
    vkGetBufferMemoryRequirements(vk.device, data.buffer, &memory_requirements);
    
    VkMemoryAllocateInfo allocate_info{};
    allocate_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocate_info.allocationSize = memory_requirements.size;
    allocate_info.memoryTypeIndex = getMemoryType(vk, memory_requirements.memoryTypeBits, properties);
    
    //vkAllocate is discouraged for many components, see https://vulkan-tutorial.com/Vertex_buffers/Staging_buffer
    if (vkAllocateMemory(vk.device, &allocate_info, nullptr, &data.memory) != VK_SUCCESS)
        log::error("Failed to allocate Buffer Memory");
    
    vkBindBufferMemory(vk.device, data.buffer, data.memory, 0);
    
    return data;
}

ui32 VK::getMemoryType(Vulkan &vk, ui32 filter, VkMemoryPropertyFlags properties) {
    VkPhysicalDeviceMemoryProperties memory_properties;
    vkGetPhysicalDeviceMemoryProperties(vk.physical_device, &memory_properties);
    
    for (ui32 i = 0; i < memory_properties.memoryTypeCount; i++) {
        if ((filter & (1 << i)) && (memory_properties.memoryTypes[i].propertyFlags & properties) == properties)
            return i;
    }
    
    log::error("Failed to find a suitable memory type");
    return 0;
}

void VK::createVertexBuffer(Vulkan &vk, const std::vector<Graphics::VertexData> &vertices) {
    VkDeviceSize buffer_size = sizeof(vertices[0]) * vertices.size();
    
    VkBufferUsageFlags staging_buffer_usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    VkMemoryPropertyFlags staging_buffer_properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    BufferData staging_buffer = createBuffer(vk, buffer_size, staging_buffer_usage, staging_buffer_properties);
    
    void* data;
    vkMapMemory(vk.device, staging_buffer.memory, 0, buffer_size, 0, &data);
    memcpy(data, vertices.data(), (size_t) buffer_size);
    vkUnmapMemory(vk.device, staging_buffer.memory);
    
    VkBufferUsageFlags buffer_usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    VkMemoryPropertyFlags buffer_properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    vk.vertex_buffer = createBuffer(vk, buffer_size, buffer_usage, buffer_properties);
    
    copyBuffer(vk, staging_buffer.buffer, vk.vertex_buffer.buffer, buffer_size);
    
    vkDestroyBuffer(vk.device, staging_buffer.buffer, nullptr);
    vkFreeMemory(vk.device, staging_buffer.memory, nullptr);
}

void VK::createIndexBuffer(Vulkan &vk, const std::vector<ui16> &indices) {
    VkDeviceSize buffer_size = sizeof(indices[0]) * indices.size();
    vk.index_buffer_size = (ui32)indices.size();
    
    VkBufferUsageFlags staging_buffer_usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    VkMemoryPropertyFlags staging_buffer_properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    BufferData staging_buffer = createBuffer(vk, buffer_size, staging_buffer_usage, staging_buffer_properties);
    
    void* data;
    vkMapMemory(vk.device, staging_buffer.memory, 0, buffer_size, 0, &data);
    memcpy(data, indices.data(), (size_t) buffer_size);
    vkUnmapMemory(vk.device, staging_buffer.memory);
    
    VkBufferUsageFlags buffer_usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
    VkMemoryPropertyFlags buffer_properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    vk.index_buffer = createBuffer(vk, buffer_size, buffer_usage, buffer_properties);
    
    copyBuffer(vk, staging_buffer.buffer, vk.index_buffer.buffer, buffer_size);
    
    vkDestroyBuffer(vk.device, staging_buffer.buffer, nullptr);
    vkFreeMemory(vk.device, staging_buffer.memory, nullptr);
}

void VK::createCommandBuffers(Vulkan &vk) {
    vk.cmd.command_buffers["draw"].resize(vk.swapchain_framebuffers.size());
    
    VkCommandBufferAllocateInfo allocate_info{};
    allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocate_info.commandPool = vk.cmd.command_pools["draw"];
    allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocate_info.commandBufferCount = (ui32)vk.cmd.command_buffers["draw"].size();
    
    if (vkAllocateCommandBuffers(vk.device, &allocate_info, vk.cmd.command_buffers["draw"].data()) != VK_SUCCESS)
        log::error("Failed to allocate a Vulkan Command Buffer");
    
    for (int i = 0; i < vk.cmd.command_buffers["draw"].size(); i++) {
        VkCommandBufferBeginInfo begin_info{};
        begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        begin_info.flags = 0;
        begin_info.pInheritanceInfo = nullptr;
        
        if (vkBeginCommandBuffer(vk.cmd.command_buffers["draw"][i], &begin_info) != VK_SUCCESS)
            log::error("Failed to create a Vulkan Command Buffer");
        
        recordCommandBuffer(vk, vk.cmd.command_buffers["draw"][i], vk.swapchain_framebuffers[i], vk.descriptor_sets[i]);
        //TODO: Check how to clean this?
        
        if (vkEndCommandBuffer(vk.cmd.command_buffers["draw"][i]) != VK_SUCCESS)
            log::error("Failed to end recording on a Vulkan Command Buffer");
    }
    
    log::graphics("Created all Vulkan Command Buffers");
}

void VK::recordCommandBuffer(Vulkan &vk, VkCommandBuffer &command_buffer, VkFramebuffer &framebuffer, VkDescriptorSet &descriptor_set) {
    std::array<VkClearValue, 2> clear_values{};
    clear_values[0].color = {0.01f, 0.01f, 0.05f, 1.0f};
    clear_values[1].depthStencil = {1.0f, 0};
    
    VkRenderPassBeginInfo render_pass_info{};
    render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    render_pass_info.renderPass = vk.render_pass;
    render_pass_info.framebuffer = framebuffer;
    render_pass_info.renderArea.offset = {0, 0};
    render_pass_info.renderArea.extent = vk.swapchain.extent;
    render_pass_info.clearValueCount = 2;
    render_pass_info.pClearValues = clear_values.data();
    
    vkCmdBeginRenderPass(command_buffer, &render_pass_info, VK_SUBPASS_CONTENTS_INLINE);
    
    vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vk.pipeline);
    
    VkBuffer vertex_buffers[]{ vk.vertex_buffer.buffer };
    VkDeviceSize offsets[]{ 0 };
    vkCmdBindVertexBuffers(command_buffer, 0, 1, vertex_buffers, offsets);
    
    vkCmdBindIndexBuffer(command_buffer, vk.index_buffer.buffer, 0, VK_INDEX_TYPE_UINT16);
    
    vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vk.pipeline_layout, 0, 1, &descriptor_set, 0, nullptr);
    
    vkCmdDrawIndexed(command_buffer, vk.index_buffer_size, 1, 0, 0, 0);
    
    vkCmdEndRenderPass(command_buffer);
}

VkCommandBuffer VK::beginSingleUseCommandBuffer(Vulkan &vk) {
    VkCommandBufferAllocateInfo allocate_info{};
    allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocate_info.commandPool = vk.cmd.command_pools["temp"];
    allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocate_info.commandBufferCount = 1;
    
    VkCommandBuffer command_buffer;
    vkAllocateCommandBuffers(vk.device, &allocate_info, &command_buffer);
    
    VkCommandBufferBeginInfo begin_info{};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(command_buffer, &begin_info);
    
    return command_buffer;
}

void VK::endSingleUseCommandBuffer(Vulkan &vk, VkCommandBuffer command_buffer) {
    vkEndCommandBuffer(command_buffer);
    
    VkSubmitInfo submit_info{};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &command_buffer;
    vkQueueSubmit(vk.cmd.queues.graphics, 1, &submit_info, VK_NULL_HANDLE);
    vkQueueWaitIdle(vk.cmd.queues.graphics);
    
    vkFreeCommandBuffers(vk.device, vk.cmd.command_pools["temp"], 1, &command_buffer);
}

void VK::copyBuffer(Vulkan &vk, VkBuffer src, VkBuffer dst, VkDeviceSize size) {
    VkCommandBuffer command_buffer = beginSingleUseCommandBuffer(vk);
    
    VkBufferCopy copy_region{};
    copy_region.srcOffset = 0;
    copy_region.dstOffset = 0;
    copy_region.size = size;
    vkCmdCopyBuffer(command_buffer, src, dst, 1, &copy_region);
    
    endSingleUseCommandBuffer(vk, command_buffer);
}

//----------------------------------------



//Uniforms
//----------------------------------------

void VK::createDescriptorPool(Vulkan &vk) {
    std::array<VkDescriptorPoolSize, 2> pool_sizes{};
    
    pool_sizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    pool_sizes[0].descriptorCount = (ui32)vk.swapchain.images.size();
    
    pool_sizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    pool_sizes[1].descriptorCount = (ui32)vk.swapchain.images.size();
    
    VkDescriptorPoolCreateInfo create_info{};
    create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    create_info.poolSizeCount = (ui32)pool_sizes.size();
    create_info.pPoolSizes = pool_sizes.data();
    create_info.maxSets = (ui32)vk.swapchain.images.size();
    
    if (vkCreateDescriptorPool(vk.device, &create_info, nullptr, &vk.descriptor_pool) != VK_SUCCESS)
        log::error("Failed to create a Vulkan Descriptor Pool");
    
    log::graphics("Created a Vulkan Descriptor Pool");
}

void VK::createDescriptorSets(Vulkan &vk) {
    std::vector<VkDescriptorSetLayout> layouts(vk.swapchain.images.size(), vk.descriptor_set_layout);
    
    VkDescriptorSetAllocateInfo allocate_info{};
    allocate_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocate_info.descriptorPool = vk.descriptor_pool;
    allocate_info.descriptorSetCount = static_cast<ui32>(vk.swapchain.images.size());
    allocate_info.pSetLayouts = layouts.data();
    
    vk.descriptor_sets.resize(vk.swapchain.images.size());
    if (vkAllocateDescriptorSets(vk.device, &allocate_info, vk.descriptor_sets.data()) != VK_SUCCESS)
        log::error("Failed to allocate Vulkan Descriptor Sets");
    
    log::graphics("Allocated Vulkan Descriptor Sets");
    
    for (int i = 0; i < vk.swapchain.images.size(); i++) {
        VkDescriptorBufferInfo buffer_info{};
        buffer_info.buffer = vk.uniform_buffers[i].buffer;
        buffer_info.offset = 0;
        buffer_info.range = VK_WHOLE_SIZE; //Also possible to use sizeof(VK::UniformBufferObject);
        
        //TODO: make this editable and not depend on vk objects
        VkDescriptorImageInfo image_info{};
        image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        image_info.imageView = vk.image_view;
        image_info.sampler = vk.sampler;
        
        std::array<VkWriteDescriptorSet, 2> write_descriptors{};
        
        write_descriptors[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write_descriptors[0].dstSet = vk.descriptor_sets[i];
        write_descriptors[0].dstBinding = 0; //Binding, specified in the shader
        write_descriptors[0].dstArrayElement = 0;
        write_descriptors[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        write_descriptors[0].descriptorCount = 1;
        write_descriptors[0].pBufferInfo = &buffer_info;
        write_descriptors[0].pImageInfo = nullptr;
        write_descriptors[0].pTexelBufferView = nullptr;
        
        write_descriptors[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write_descriptors[1].dstSet = vk.descriptor_sets[i];
        write_descriptors[1].dstBinding = 1;
        write_descriptors[1].dstArrayElement = 0;
        write_descriptors[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        write_descriptors[1].descriptorCount = 1;
        write_descriptors[1].pBufferInfo = nullptr;
        write_descriptors[1].pImageInfo = &image_info;
        write_descriptors[1].pTexelBufferView = nullptr;
        
        vkUpdateDescriptorSets(vk.device, (ui32)write_descriptors.size(), write_descriptors.data(), 0, nullptr);
    }
}

void VK::createUniformBuffers(Vulkan &vk) {
    VkDeviceSize buffer_size = sizeof(VK::UniformBufferObject);
    
    vk.uniform_buffers.resize(vk.swapchain.images.size());
    
    VkBufferUsageFlags usage_flags = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    VkMemoryPropertyFlags memory_flags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    
    for (int i = 0; i < vk.swapchain.images.size(); i++)
        vk.uniform_buffers[i] = createBuffer(vk, buffer_size, usage_flags, memory_flags);
    
    log::graphics("Created Vulkan Uniform Buffers");
}

void VK::updateUniformBuffer(Vulkan &vk, ui32 current_image) {
    //EXAMPLE FUNCTION
    
    static Clock::time_point start_time = time();
    float t = sec(time() - start_time);
    
    VK::UniformBufferObject ubo{};
    
    ubo.model = glm::scale(glm::mat4(1.0f), glm::vec3(0.5f, 0.5f, 0.5f));
    ubo.model = glm::rotate(ubo.model, t * 1.570796f, glm::vec3(0.0f, 0.0f, 1.0f));
    ubo.model = glm::translate(ubo.model, glm::vec3(0.0f, 0.0f, 0.3f * std::sin(t * 1.570796f)));
    ubo.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    ubo.proj = glm::perspective(glm::radians(45.0f), vk.swapchain.extent.width / (float) vk.swapchain.extent.height, 0.1f, 10.0f);
    ubo.proj[1][1] *= -1;
    
    void* data;
    vkMapMemory(vk.device, vk.uniform_buffers[current_image].memory, 0, sizeof(ubo), 0, &data);
    memcpy(data, &ubo, sizeof(ubo));
    vkUnmapMemory(vk.device, vk.uniform_buffers[current_image].memory);
}

//----------------------------------------



//Images
//----------------------------------------

void API::createTexture(Vulkan &vk, TextureData &tex, ui8 *pixels) {
    ui32 size = tex.w * tex.h * tex.ch * sizeof(ui8);
    
    VkBufferUsageFlags usage_flags = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    VkMemoryPropertyFlags memory_flags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    BufferData staging_buffer = VK::createBuffer(vk, size, usage_flags, memory_flags);
    
    void* data;
    vkMapMemory(vk.device, staging_buffer.memory, 0, size, 0, &data);
    memcpy(data, pixels, static_cast<size_t>(size));
    vkUnmapMemory(vk.device, staging_buffer.memory);
    
    VK::createImage(vk, tex, pixels);
    VK::transitionImageLayout(vk, tex, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    VK::copyBufferToImage(vk, staging_buffer, tex);
    VK::transitionImageLayout(vk, tex, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    
    vk.image_view = VK::createImageView(vk.device, tex.image, VK_IMAGE_ASPECT_COLOR_BIT, tex.format);
    
    vkDestroyBuffer(vk.device, staging_buffer.buffer, nullptr);
    vkFreeMemory(vk.device, staging_buffer.memory, nullptr);
}

void VK::createImage(Vulkan &vk, TextureData &tex, ui8 *pixels) {
    VkImageCreateInfo create_info{};
    create_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    create_info.imageType = VK_IMAGE_TYPE_2D;
    create_info.extent.width = static_cast<ui32>(tex.w);
    create_info.extent.height = static_cast<ui32>(tex.h);
    create_info.extent.depth = 1;
    create_info.mipLevels = 1;
    create_info.arrayLayers = 1;
    
    //This way the image can't be access directly, but it is better for performance
    create_info.tiling = VK_IMAGE_TILING_OPTIMAL;
    
    tex.format = [tex](){
        switch(tex.ch) {
            case 1:
                return VK_FORMAT_R8_SRGB;
            case 2:
                return VK_FORMAT_R8G8_SRGB;
            case 3:
                return VK_FORMAT_R8G8B8_SRGB;
            default:
                return VK_FORMAT_R8G8B8A8_SRGB;
        }
        //Is it necessary to test for alternatives?
    }();
    create_info.format = tex.format;
    
    tex.layout = VK_IMAGE_LAYOUT_UNDEFINED;
    create_info.initialLayout = tex.layout;
    
    create_info.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    
    create_info.samples = VK_SAMPLE_COUNT_1_BIT;
    create_info.flags = 0;
    
    if (vkCreateImage(vk.device, &create_info, nullptr, &tex.image) != VK_SUCCESS)
        log::error("Failed to create a Vulkan image");
    
    
    VkMemoryRequirements memory_requirements;
    vkGetImageMemoryRequirements(vk.device, tex.image, &memory_requirements);
    
    VkMemoryAllocateInfo allocate_info{};
    allocate_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocate_info.allocationSize = memory_requirements.size;
    allocate_info.memoryTypeIndex = getMemoryType(vk, memory_requirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    
    if (vkAllocateMemory(vk.device, &allocate_info, nullptr, &tex.memory) != VK_SUCCESS)
        log::error("Failed to allocate memory for a Vulkan image");
    
    vkBindImageMemory(vk.device, tex.image, tex.memory, 0);
}

void VK::transitionImageLayout(Vulkan &vk, TextureData &tex, VkImageLayout new_layout) {
    VkCommandBuffer command_buffer = beginSingleUseCommandBuffer(vk);

    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.image = tex.image;
    
    barrier.oldLayout = tex.layout;
    barrier.newLayout = new_layout;
    
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;
    
    VkPipelineStageFlags src_stage;
    VkPipelineStageFlags dst_stage;
    
    barrier.srcAccessMask = [tex, &src_stage](){
        switch (tex.layout) {
            case VK_IMAGE_LAYOUT_UNDEFINED:
                src_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
                return (VkAccessFlagBits)0;
            case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
                src_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
                return VK_ACCESS_TRANSFER_WRITE_BIT;
            default:
                log::warn("Not a valid src access mask in image layout transition");
                return (VkAccessFlagBits)0;
        }
    }();
    
    barrier.dstAccessMask = [new_layout, &dst_stage](){
        switch (new_layout) {
            case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
                dst_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
                return VK_ACCESS_TRANSFER_WRITE_BIT;
            case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
                dst_stage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
                return VK_ACCESS_SHADER_READ_BIT;
            default:
                log::warn("Not a valid dst access mask in image layout transition");
                return (VkAccessFlagBits)0;
        }
    }();
    
    vkCmdPipelineBarrier(command_buffer, src_stage, dst_stage, 0, 0, nullptr, 0, nullptr, 1, &barrier); //TODO
    
    endSingleUseCommandBuffer(vk, command_buffer);
    
    tex.layout = new_layout;
}

void VK::copyBufferToImage(Vulkan &vk, BufferData &buffer, TextureData &tex) {
    VkCommandBuffer command_buffer = beginSingleUseCommandBuffer(vk);
    
    VkBufferImageCopy copy_region{};
    copy_region.bufferOffset = 0;
    copy_region.bufferRowLength = 0;
    copy_region.bufferImageHeight = 0;

    copy_region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    copy_region.imageSubresource.mipLevel = 0;
    copy_region.imageSubresource.baseArrayLayer = 0;
    copy_region.imageSubresource.layerCount = 1;

    copy_region.imageOffset = {0, 0, 0};
    copy_region.imageExtent = {(ui32)tex.w, (ui32)tex.h, 1};
    
    vkCmdCopyBufferToImage(command_buffer, buffer.buffer, tex.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copy_region);
    
    endSingleUseCommandBuffer(vk, command_buffer);
}

VkImageView VK::createImageView(VkDevice &device, VkImage image, VkImageAspectFlags aspect_flags, VkFormat format) {
    VkImageViewCreateInfo create_info{};
    
    create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    create_info.image = image;
    
    create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
    create_info.format = format;
    
    create_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    create_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    create_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    create_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
    
    create_info.subresourceRange.aspectMask = aspect_flags;
    create_info.subresourceRange.baseMipLevel = 0;
    create_info.subresourceRange.levelCount = 1;
    create_info.subresourceRange.baseArrayLayer = 0;
    create_info.subresourceRange.layerCount = 1;
    
    VkImageView image_view;
    if (vkCreateImageView(device, &create_info, nullptr, &image_view)!= VK_SUCCESS)
        log::error("Error creating a Vulkan image view");
    
    return image_view;
}

void VK::createSampler(Vulkan &vk) {
    VkSamplerCreateInfo create_info{};
    create_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    
    create_info.magFilter = VK_FILTER_NEAREST; //This is for pixel art, change to linear for interpolation
    create_info.minFilter = VK_FILTER_NEAREST;
    
    create_info.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER; //The texture doesn't repeat if sampled out of range
    create_info.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
    create_info.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
    create_info.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    
    create_info.anisotropyEnable = VK_FALSE;
    create_info.maxAnisotropy = 1.0f;
    
    //Enable for anisotropy, as well as device features in createDevice()
    /*VkPhysicalDeviceProperties properties{};
    vkGetPhysicalDeviceProperties(vk.physical_device, &properties);
    create_info.maxAnisotropy = properties.limits.maxSamplerAnisotropy;*/
    
    create_info.unnormalizedCoordinates = VK_FALSE;
    create_info.compareEnable = VK_FALSE;
    create_info.compareOp = VK_COMPARE_OP_ALWAYS;
    
    create_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    create_info.mipLodBias = 0.0f;
    create_info.minLod = 0.0f;
    create_info.maxLod = 0.0f;
    
    if (vkCreateSampler(vk.device, &create_info, nullptr, &vk.sampler) != VK_SUCCESS)
        log::error("Error creating a Vulkan image sampler");
}

//----------------------------------------



//Framebuffers
//----------------------------------------

void VK::createFramebuffers(Vulkan &vk) {
    vk.swapchain_framebuffers.resize(vk.swapchain.image_views.size());
    
    for (int i = 0; i < vk.swapchain_framebuffers.size(); i++) {
        VkFramebufferCreateInfo create_info{};
        create_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        create_info.renderPass = vk.render_pass;
        create_info.attachmentCount = 1;
        create_info.pAttachments = &vk.swapchain.image_views[i];
        create_info.width = vk.swapchain.extent.width;
        create_info.height = vk.swapchain.extent.height;
        create_info.layers = 1;
        
        if (vkCreateFramebuffer(vk.device, &create_info, nullptr, &vk.swapchain_framebuffers[i]) != VK_SUCCESS)
            log::error("Failed to create a Vulkan Framebuffer");
    }
    
    log::graphics("Created all Vulkan Framebuffers");
}

//----------------------------------------



//Sync objects
//----------------------------------------

void VK::createSyncObjects(Vulkan &vk) {
    vk.semaphores_image_available.resize(MAX_FRAMES_IN_FLIGHT);
    vk.semaphores_render_finished.resize(MAX_FRAMES_IN_FLIGHT);
    vk.fences_in_flight.resize(MAX_FRAMES_IN_FLIGHT);
    vk.fences_images_in_flight.resize(vk.swapchain.images.size(), VK_NULL_HANDLE);
    
    VkSemaphoreCreateInfo semaphore_info{};
    semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    
    VkFenceCreateInfo fence_info{};
    fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;
    
    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        if (vkCreateSemaphore(vk.device, &semaphore_info, nullptr, &vk.semaphores_image_available[i]) != VK_SUCCESS)
            log::error("Failed to create a Vulkan Semaphore");
        
        if (vkCreateSemaphore(vk.device, &semaphore_info, nullptr, &vk.semaphores_render_finished[i]) != VK_SUCCESS)
            log::error("Failed to create a Vulkan Semaphore");
        
        if (vkCreateFence(vk.device, &fence_info, nullptr, &vk.fences_in_flight[i]) != VK_SUCCESS)
            log::error("Failed to create a Vulkan Fence");
    }
    
    log::graphics("Created all Vulkan Semaphores");
}

//----------------------------------------



//Render
//----------------------------------------

void VK::renderFrame(Vulkan &vk, WindowData &win) {
    vkWaitForFences(vk.device, 1, &vk.fences_in_flight[vk.current_frame], VK_TRUE, UINT64_MAX);
    
    VkResult result;
    ui32 image_index;
    result = vkAcquireNextImageKHR(vk.device, vk.swapchain.swapchain, UINT64_MAX, vk.semaphores_image_available[vk.current_frame], VK_NULL_HANDLE, &image_index);
    
    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        recreateSwapchain(vk, win);
        return;
    }
    if (result != VK_SUCCESS and result != VK_SUBOPTIMAL_KHR) {
        log::error("Failed to acquire Swapchain Image");
    }
    
    
    if (vk.fences_images_in_flight[image_index] != VK_NULL_HANDLE)
        vkWaitForFences(vk.device, 1, &vk.fences_images_in_flight[image_index], VK_TRUE, UINT64_MAX);
    vk.fences_images_in_flight[image_index] = vk.fences_in_flight[vk.current_frame];
    
    VkSemaphore wait_semaphores[]{ vk.semaphores_image_available[vk.current_frame] };
    VkPipelineStageFlags wait_stages[]{ VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
    VkSemaphore signal_semaphores[]{ vk.semaphores_render_finished[vk.current_frame] };
    
    
    updateUniformBuffer(vk, image_index);
    
    
    VkSubmitInfo submit_info{};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    
    submit_info.waitSemaphoreCount = 1;
    submit_info.pWaitSemaphores = wait_semaphores;
    submit_info.pWaitDstStageMask = wait_stages;
    
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &vk.cmd.command_buffers["draw"][image_index];
    
    submit_info.signalSemaphoreCount = 1;
    submit_info.pSignalSemaphores = signal_semaphores;
    
    
    vkResetFences(vk.device, 1, &vk.fences_in_flight[vk.current_frame]);
    
    if (vkQueueSubmit(vk.cmd.queues.graphics, 1, &submit_info, vk.fences_in_flight[vk.current_frame]) != VK_SUCCESS)
        log::error("Failed to submit Draw Command Buffer");
    
    
    VkPresentInfoKHR present_info{};
    present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

    present_info.waitSemaphoreCount = 1;
    present_info.pWaitSemaphores = signal_semaphores;
    
    VkSwapchainKHR swapchains[]{ vk.swapchain.swapchain };
    present_info.swapchainCount = 1;
    present_info.pSwapchains = swapchains;
    present_info.pImageIndices = &image_index;
    present_info.pResults = nullptr;
    
    
    result = vkQueuePresentKHR(vk.cmd.queues.present, &present_info);
    
    if (result == VK_ERROR_OUT_OF_DATE_KHR or result == VK_SUBOPTIMAL_KHR)
        recreateSwapchain(vk, win);
    else if (result != VK_SUCCESS)
        log::error("Failed to present Swapchain Image");
    
    
    vk.current_frame = (vk.current_frame + 1) % MAX_FRAMES_IN_FLIGHT;
}

//----------------------------------------


//Test
//----------------------------------------

void API::renderTest(WindowData &win, RenderData &render) {
    VK::renderFrame(render.api, win);
}

//----------------------------------------



//Resize
//----------------------------------------

void API::resize(Vulkan &vk, WindowData &win) {
    VK::recreateSwapchain(vk, win);
}

//----------------------------------------



//Cleanup
//----------------------------------------

void VK::cleanSwapchain(Vulkan &vk) {
    for (VkFramebuffer fb : vk.swapchain_framebuffers)
        vkDestroyFramebuffer(vk.device, fb, nullptr);
    
    for (auto [key, val] : vk.cmd.command_buffers)
        vkFreeCommandBuffers(vk.device, vk.cmd.command_pools[key], (ui32)val.size(), val.data());
    
    vkDestroyPipeline(vk.device, vk.pipeline, nullptr);
    vkDestroyPipelineLayout(vk.device, vk.pipeline_layout, nullptr);
    vkDestroyRenderPass(vk.device, vk.render_pass, nullptr);
    
    for (VkImageView view : vk.swapchain.image_views)
        vkDestroyImageView(vk.device, view, nullptr);
    
    vkDestroySwapchainKHR(vk.device, vk.swapchain.swapchain, nullptr);
    
    for (BufferData buffer : vk.uniform_buffers) {
        vkDestroyBuffer(vk.device, buffer.buffer, nullptr);
        vkFreeMemory(vk.device, buffer.memory, nullptr);
    }
    
    vkDestroyDescriptorPool(vk.device, vk.descriptor_pool, nullptr);
}

void API::clean(Vulkan &vk) {
    vkDeviceWaitIdle(vk.device);
    
    VK::cleanSwapchain(vk);
    
    vkDestroyImageView(vk.device, vk.image_view, nullptr);
    vkDestroyImage(vk.device, vk.test_image.image, nullptr);
    vkFreeMemory(vk.device, vk.test_image.memory, nullptr);
    vkDestroySampler(vk.device, vk.sampler, nullptr);
    
    vkDestroyDescriptorSetLayout(vk.device, vk.descriptor_set_layout, nullptr);
    
    vkDestroyBuffer(vk.device, vk.vertex_buffer.buffer, nullptr);
    vkFreeMemory(vk.device, vk.vertex_buffer.memory, nullptr);
    
    vkDestroyBuffer(vk.device, vk.index_buffer.buffer, nullptr);
    vkFreeMemory(vk.device, vk.index_buffer.memory, nullptr);
    
    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        vkDestroySemaphore(vk.device, vk.semaphores_image_available[i], nullptr);
        vkDestroySemaphore(vk.device, vk.semaphores_render_finished[i], nullptr);
        vkDestroyFence(vk.device, vk.fences_in_flight[i], nullptr);
    }
    
    for (auto [key, val] : vk.cmd.command_pools)
        vkDestroyCommandPool(vk.device, val, nullptr);
    
    vkDestroyDevice(vk.device, nullptr);
    
    vkDestroySurfaceKHR(vk.instance, vk.surface, nullptr);
    vkDestroyInstance(vk.instance, nullptr);
    
    log::graphics("Cleaned up Vulkan");
}

//----------------------------------------



//DEBUG
//----------------------------------------

static VKAPI_ATTR VkBool32 VKAPI_CALL vulkanReportFunc(
    VkDebugReportFlagsEXT flags,
    VkDebugReportObjectTypeEXT objType,
    uint64_t obj,
    size_t location,
    int32_t code,
    const char* layerPrefix,
    const char* msg,
    void* userData)

{
    printf("[VULKAN]: %s\n", msg);
    return VK_FALSE;
}

PFN_vkCreateDebugReportCallbackEXT SDL2_vkCreateDebugReportCallbackEXT = nullptr;

VkDebugReportCallbackEXT VK::createDebug(VkInstance &instance) {
    SDL2_vkCreateDebugReportCallbackEXT = (PFN_vkCreateDebugReportCallbackEXT)SDL_Vulkan_GetVkGetInstanceProcAddr();

    VkDebugReportCallbackCreateInfoEXT debug_callback_create_info = {};
    debug_callback_create_info.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
    debug_callback_create_info.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT;
    debug_callback_create_info.pfnCallback = vulkanReportFunc;

    VkDebugReportCallbackEXT debug_callback;
    
    SDL2_vkCreateDebugReportCallbackEXT(instance, &debug_callback_create_info, 0, &debug_callback);
    
    return debug_callback;
}

//----------------------------------------

#endif
