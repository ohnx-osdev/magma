// Copyright 2016 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#if defined(MAGMA_USE_SHIM)
#include "vulkan_shim.h"
#else
#include <vulkan/vulkan.h>
#endif
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vector>

#include "magma_util/dlog.h"
#include "magma_util/macros.h"

class VkReadbackTest {
public:
    static constexpr uint32_t kWidth = 64;
    static constexpr uint32_t kHeight = 64;

    bool Initialize();
    bool Exec();
    bool Readback();

#if defined(MAGMA_TEST_IMPORT_EXPORT)
    uint32_t get_device_memory_handle() { return device_memory_handle_; }
    void set_device_memory_handle(uint32_t handle) { device_memory_handle_ = handle; }
#endif

private:
    bool InitVulkan();
    bool InitImage();

    bool is_initialized_ = false;
    VkPhysicalDevice vk_physical_device_;
    VkDevice vk_device_;
    VkQueue vk_queue_;
    VkImage vk_image_;
    VkDeviceMemory vk_device_memory_;
#if defined(MAGMA_TEST_IMPORT_EXPORT)
    VkDeviceMemory vk_imported_device_memory_ = VK_NULL_HANDLE;
    uint32_t device_memory_handle_ = 0;
#endif
    VkCommandPool vk_command_pool_;
    VkCommandBuffer vk_command_buffer_;
};

bool VkReadbackTest::Initialize()
{
    if (is_initialized_)
        return false;

    if (!InitVulkan())
        return DRETF(false, "failed to initialize Vulkan");

    if (!InitImage())
        return DRETF(false, "InitImage failed");

    is_initialized_ = true;

    return true;
}

bool VkReadbackTest::InitVulkan()
{
    VkInstanceCreateInfo create_info{
        VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO, // VkStructureType             sType;
        nullptr,                                // const void*                 pNext;
        0,                                      // VkInstanceCreateFlags       flags;
        nullptr,                                // const VkApplicationInfo*    pApplicationInfo;
        0,                                      // uint32_t                    enabledLayerCount;
        nullptr,                                // const char* const*          ppEnabledLayerNames;
        0,       // uint32_t                    enabledExtensionCount;
        nullptr, // const char* const*          ppEnabledExtensionNames;
    };
    VkAllocationCallbacks* allocation_callbacks = nullptr;
    VkInstance instance;
    VkResult result;

    if ((result = vkCreateInstance(&create_info, allocation_callbacks, &instance)) != VK_SUCCESS)
        return DRETF(false, "vkCreateInstance failed %d", result);

    DLOG("vkCreateInstance succeeded");

    uint32_t physical_device_count;
    if ((result = vkEnumeratePhysicalDevices(instance, &physical_device_count, nullptr)) !=
        VK_SUCCESS)
        return DRETF(false, "vkEnumeratePhysicalDevices failed %d", result);

    if (physical_device_count < 1)
        return DRETF(false, "unexpected physical_device_count %d", physical_device_count);

    DLOG("vkEnumeratePhysicalDevices returned count %d", physical_device_count);

    std::vector<VkPhysicalDevice> physical_devices(physical_device_count);
    if ((result = vkEnumeratePhysicalDevices(instance, &physical_device_count,
                                             physical_devices.data())) != VK_SUCCESS)
        return DRETF(false, "vkEnumeratePhysicalDevices failed %d", result);

    for (auto device : physical_devices) {
        VkPhysicalDeviceProperties properties;
        vkGetPhysicalDeviceProperties(device, &properties);
        DLOG("PHYSICAL DEVICE: %s", properties.deviceName);
        DLOG("apiVersion 0x%x", properties.apiVersion);
        DLOG("driverVersion 0x%x", properties.driverVersion);
        DLOG("vendorID 0x%x", properties.vendorID);
        DLOG("deviceID 0x%x", properties.deviceID);
        DLOG("deviceType 0x%x", properties.deviceType);
    }

    uint32_t queue_family_count;
    vkGetPhysicalDeviceQueueFamilyProperties(physical_devices[0], &queue_family_count, nullptr);

    if (queue_family_count < 1)
        return DRETF(false, "invalid queue_family_count %d", queue_family_count);

    std::vector<VkQueueFamilyProperties> queue_family_properties(queue_family_count);
    vkGetPhysicalDeviceQueueFamilyProperties(physical_devices[0], &queue_family_count,
                                             queue_family_properties.data());

    int32_t queue_family_index = -1;
    for (uint32_t i = 0; i < queue_family_count; i++) {
        if (queue_family_properties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            queue_family_index = i;
            break;
        }
    }

    if (queue_family_index < 0)
        return DRETF(false, "couldn't find an appropriate queue");

    float queue_priorities[1] = {0.0};

    VkDeviceQueueCreateInfo queue_create_info = {.sType =
                                                     VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
                                                     .pNext = nullptr,
                                                     .flags = 0,
                                                     .queueFamilyIndex = 0,
                                                 .queueCount = 1,
                                                 .pQueuePriorities = queue_priorities};
    VkDeviceCreateInfo createInfo = {.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
                                     .pNext = nullptr,
                                     .flags = 0,
                                     .queueCreateInfoCount = 1,
                                     .pQueueCreateInfos = &queue_create_info,
                                     .enabledLayerCount = 0,
                                     .ppEnabledLayerNames = nullptr,
                                     .enabledExtensionCount = 0,
                                     .ppEnabledExtensionNames = nullptr,
                                     .pEnabledFeatures = nullptr};
    VkDevice vkdevice;

    if ((result = vkCreateDevice(physical_devices[0], &createInfo,
                                 nullptr /* allocationcallbacks */, &vkdevice)) != VK_SUCCESS)
        return DRETF(false, "vkCreateDevice failed: %d", result);

    vk_physical_device_ = physical_devices[0];
    vk_device_ = vkdevice;

    vkGetDeviceQueue(vkdevice, queue_family_index, 0, &vk_queue_);

    return true;
}

bool VkReadbackTest::InitImage()
{
    VkImageCreateInfo image_create_info = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .pNext = nullptr,
        .flags = VK_IMAGE_CREATE_MUTABLE_FORMAT_BIT,
        .imageType = VK_IMAGE_TYPE_2D,
        .format = VK_FORMAT_R8G8B8A8_UNORM,
        .extent = VkExtent3D{kWidth, kHeight, 1},
        .mipLevels = 1,
        .arrayLayers = 1,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .tiling = VK_IMAGE_TILING_LINEAR,
        .usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .queueFamilyIndexCount = 0,     // not used since not sharing
        .pQueueFamilyIndices = nullptr, // not used since not sharing
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
    };

    VkResult result;

    if ((result = vkCreateImage(vk_device_, &image_create_info, nullptr, &vk_image_)) != VK_SUCCESS)
        return DRETF(false, "vkCreateImage failed: %d", result);

    DLOG("Created image");

    VkMemoryRequirements memory_reqs;
    vkGetImageMemoryRequirements(vk_device_, vk_image_, &memory_reqs);

    VkPhysicalDeviceMemoryProperties memory_props;
    vkGetPhysicalDeviceMemoryProperties(vk_physical_device_, &memory_props);

    uint32_t memory_type = 0;
    for (; memory_type < 32; memory_type++) {
        if ((memory_reqs.memoryTypeBits & (1 << memory_type)) &&
            (memory_props.memoryTypes[memory_type].propertyFlags &
             VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT))
            break;
    }
    if (memory_type >= 32)
        return DRETF(false, "Can't find compatible mappable memory for image");

    VkMemoryAllocateInfo alloc_info = {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .pNext = nullptr,
        .allocationSize = memory_reqs.size,
        .memoryTypeIndex = memory_type,
    };

    if ((result = vkAllocateMemory(vk_device_, &alloc_info, nullptr, &vk_device_memory_)) !=
        VK_SUCCESS)
        return DRETF(false, "vkAllocateMemory failed");

#if defined(MAGMA_TEST_IMPORT_EXPORT)
    if (device_memory_handle_) {
        DASSERT(vkImportDeviceMemoryMAGMA);
        if ((result = vkImportDeviceMemoryMAGMA(vk_device_, device_memory_handle_, nullptr,
                                                &vk_imported_device_memory_)) != VK_SUCCESS)
            return DRETF(false, "vkImportDeviceMemoryMAGMA failed");
        printf("imported from handle 0x%x\n", device_memory_handle_);
    } else {
        DASSERT(vkExportDeviceMemoryMAGMA);
        if ((result = vkExportDeviceMemoryMAGMA(vk_device_, vk_device_memory_,
                                                &device_memory_handle_)) != VK_SUCCESS)
            return DRETF(false, "vkExportDeviceMemoryMAGMA failed");
        printf("exported handle 0x%x\n", device_memory_handle_);
    }
#endif

    void* addr;
    if ((result = vkMapMemory(vk_device_, vk_device_memory_, 0, VK_WHOLE_SIZE, 0, &addr)) !=
        VK_SUCCESS)
        return DRETF(false, "vkMapMeory failed: %d", result);

    memset(addr, 0xab, memory_reqs.size);

    vkUnmapMemory(vk_device_, vk_device_memory_);

    DLOG("Allocated memory for image");

    if ((result = vkBindImageMemory(vk_device_, vk_image_, vk_device_memory_, 0)) != VK_SUCCESS)
        return DRETF(false, "vkBindImageMemory failed");

    DLOG("Bound memory to image");

    VkCommandPoolCreateInfo command_pool_create_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .queueFamilyIndex = 0,
    };
    if ((result = vkCreateCommandPool(vk_device_, &command_pool_create_info, nullptr,
                                      &vk_command_pool_)) != VK_SUCCESS)
        return DRETF(false, "vkCreateCommandPool failed: %d", result);

    DLOG("Created command buffer pool");

    VkCommandBufferAllocateInfo command_buffer_create_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .pNext = nullptr,
        .commandPool = vk_command_pool_,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = 1};
    if ((result = vkAllocateCommandBuffers(vk_device_, &command_buffer_create_info,
                                           &vk_command_buffer_)) != VK_SUCCESS)
        return DRETF(false, "vkAllocateCommandBuffers failed: %d", result);

    DLOG("Created command buffer");

    VkCommandBufferBeginInfo begin_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .pNext = nullptr,
        .flags = 0,
        .pInheritanceInfo = nullptr, // ignored for primary buffers
    };
    if ((result = vkBeginCommandBuffer(vk_command_buffer_, &begin_info)) != VK_SUCCESS)
        return DRETF(false, "vkBeginCommandBuffer failed: %d", result);

    DLOG("Command buffer begin");

    VkClearColorValue color_value = {.float32 = {1.0f, 0.0f, 0.5f, 0.75f}};

    VkImageSubresourceRange image_subres_range = {
        .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
        .baseMipLevel = 0,
        .levelCount = 1,
        .baseArrayLayer = 0,
        .layerCount = 1,
    };

    vkCmdClearColorImage(vk_command_buffer_, vk_image_, VK_IMAGE_LAYOUT_GENERAL, &color_value, 1,
                         &image_subres_range);

    if ((result = vkEndCommandBuffer(vk_command_buffer_)) != VK_SUCCESS)
        return DRETF(false, "vkEndCommandBuffer failed: %d", result);

    DLOG("Command buffer end");

    return true;
}

bool VkReadbackTest::Exec()
{
    VkSubmitInfo submit_info = {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .pNext = nullptr,
        .waitSemaphoreCount = 0,
        .pWaitSemaphores = nullptr,
        .pWaitDstStageMask = nullptr,
        .commandBufferCount = 1,
        .pCommandBuffers = &vk_command_buffer_,
        .signalSemaphoreCount = 0,
        .pSignalSemaphores = nullptr,
    };

    VkResult result;
    if ((result = vkQueueSubmit(vk_queue_, 1, &submit_info, VK_NULL_HANDLE)) != VK_SUCCESS)
        return DRETF(false, "vkQueueSubmit failed");

    vkQueueWaitIdle(vk_queue_);

    return true;
}

bool VkReadbackTest::Readback()
{
    VkResult result;
    void* addr;

#if defined(MAGMA_TEST_IMPORT_EXPORT)
    VkDeviceMemory vk_device_memory = vk_imported_device_memory_;
#else
    VkDeviceMemory vk_device_memory = vk_device_memory_;
#endif
    if ((result = vkMapMemory(vk_device_, vk_device_memory, 0, VK_WHOLE_SIZE, 0, &addr)) !=
        VK_SUCCESS)
        return DRETF(false, "vkMapMeory failed: %d", result);

    auto data = reinterpret_cast<uint32_t*>(addr);

    uint32_t expected_value = 0xBF8000FF;
    uint32_t mismatches = 0;
    for (uint32_t i = 0; i < kWidth * kHeight; i++) {
        if (data[i] != expected_value) {
            if (mismatches++ < 10)
                magma::log(magma::LOG_WARNING,
                           "Value Mismatch at index %d - expected 0x%04x, got 0x%08x", i,
                           expected_value, data[i]);
        }
    }
    if (mismatches) {
        magma::log(magma::LOG_WARNING, "****** Test Failed! %d mismatches", mismatches);
    } else {
        magma::log(magma::LOG_INFO, "****** Test Passed! All values matched.");
    }

    vkUnmapMemory(vk_device_, vk_device_memory);

    return mismatches == 0;
}

int main(void)
{
#if defined(MAGMA_USE_SHIM)
    VulkanShimInit();
#endif

#if defined(MAGMA_TEST_IMPORT_EXPORT)
    VkReadbackTest export_app;
    VkReadbackTest import_app;

    if (!export_app.Initialize())
        return DRET_MSG(-1, "could not initialize export app");

    import_app.set_device_memory_handle(export_app.get_device_memory_handle());

    if (!import_app.Initialize())
        return DRET_MSG(-1, "could not initialize import app");

    if (!export_app.Exec())
        return DRET_MSG(-1, "Exec failed");

    if (!import_app.Readback())
        return DRET_MSG(-1, "Readback failed");
#else
    VkReadbackTest app;

    if (!app.Initialize())
        return DRET_MSG(-1, "could not initialize app");

    if (!app.Exec())
        return DRET_MSG(-1, "Exec failed");

    if (!app.Readback())
        return DRET_MSG(-1, "Readback failed");
#endif

    return 0;
}
