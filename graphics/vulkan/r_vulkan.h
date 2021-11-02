//project verse, 2017-2021
//by jose pazos perez
//all rights reserved uwu

#pragma once

#ifdef USE_VULKAN

#include <vulkan/vulkan.h>
#include <SDL2/SDL_vulkan.h>

#include <vector>
#include <map>

#include "r_vulkan_datatypes.h"
#include "r_bufferdata.h"
#include "r_texturedata.h"

namespace Verse::Graphics
{
    struct VkCommandData {
        std::map<str, VkCommandPool> command_pools;
        std::map<str, std::vector<VkCommandBuffer>> command_buffers;
        
        VK::QueueIndices queue_indices;
        VK::QueueData queues;
    };

    struct Vulkan {
        //Device
        //----------------------------------------
        VkInstance instance;
        std::vector<VkExtensionProperties> instance_extensions;
        
        VkSurfaceKHR surface;
        
        VkPhysicalDevice physical_device;
        VkPhysicalDeviceFeatures physical_device_features;
        
        VkDevice device;
        //----------------------------------------
        
        
        //Commands
        //----------------------------------------
        VkCommandData cmd;
        //----------------------------------------
        
        
        //Swapchain
        //----------------------------------------
        VkFormat swapchain_format;
        VkExtent2D swapchain_extent;
        
        VkSwapchainKHR swapchain;
        
        std::vector<VkImage> swapchain_images;
        std::vector<VkImageView> swapchain_image_views;
        
        VkImage depth_image;
        VkDeviceMemory depth_image_memory;
        VkImageView depth_image_view;
        //----------------------------------------
        
        
        //Render Pass
        //----------------------------------------
        VkRenderPass render_pass;
        //----------------------------------------
        
        
        //Pipeline
        //----------------------------------------
        VkDescriptorSetLayout descriptor_set_layout;
        VkPipelineLayout pipeline_layout;
        
        VkPipeline pipeline;
        //----------------------------------------
        
        
        //Buffers
        //----------------------------------------
        BufferData vertex_buffer;
        BufferData index_buffer;
        ui32 index_buffer_size;
        //----------------------------------------
        
        
        //Uniforms
        //----------------------------------------
        std::vector<BufferData> uniform_buffers;
        VkDescriptorPool descriptor_pool;
        std::vector<VkDescriptorSet> descriptor_sets;
        //----------------------------------------
        
        
        //Images
        //----------------------------------------
        TextureData test_image;
        VkImageView image_view;
        VkSampler sampler;
        //----------------------------------------
        
        
        //Framebuffers
        //----------------------------------------
        std::vector<VkFramebuffer> swapchain_framebuffers;
        //----------------------------------------
        
        
        //Sync objects
        //----------------------------------------
        std::vector<VkSemaphore> semaphores_image_available;
        std::vector<VkSemaphore> semaphores_render_finished;
        std::vector<VkFence> fences_in_flight;
        std::vector<VkFence> fences_images_in_flight;
        //----------------------------------------
        
        
        //Render
        //----------------------------------------
        ui8 current_frame;
        //----------------------------------------
        
        
        //Debug
        //----------------------------------------
        VkDebugReportCallbackEXT debug_callback;
        //----------------------------------------
    };
}


#endif


