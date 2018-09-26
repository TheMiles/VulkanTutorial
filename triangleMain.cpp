#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <iostream>
#include <stdexcept>
#include <functional>
#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <vector>
#include <fstream>
#include <set>
#include <optional>


const int WIDTH   = 800;
const int HEIGHT  = 600;
const char* TITLE = "Vulkan";

const char* DEBUG_EXTENSION = "VK_EXT_debug_report";

const std::vector<const char*> requestedExtensions = {
#ifndef NDEBUG
    DEBUG_EXTENSION
#endif
};

const std::vector<const char*> validationLayers = {
#ifndef NDEBUG
    "VK_LAYER_LUNARG_standard_validation"
#endif
};

const std::vector<const char*> deviceExtensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME
};


VkResult CreateDebugReportCallbackEXT(VkInstance& instance, const VkDebugReportCallbackCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugReportCallbackEXT* pCallback)
{
    auto func = (PFN_vkCreateDebugReportCallbackEXT) vkGetInstanceProcAddr(instance, "vkCreateDebugReportCallbackEXT");
    if (func != nullptr) {
        return func(instance, pCreateInfo, pAllocator, pCallback);
    } else {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
}

void DestroyDebugReportCallbackEXT(VkInstance& instance, VkDebugReportCallbackEXT& callback, const VkAllocationCallbacks* pAllocator = nullptr)
{
    auto func = (PFN_vkDestroyDebugReportCallbackEXT) vkGetInstanceProcAddr(instance, "vkDestroyDebugReportCallbackEXT");
    if(func)
    {
        func(instance, callback, pAllocator);
    }
}

struct QueueFamilyIndices
{
    std::optional<uint32_t> graphicsFamily;
    std::optional<uint32_t> presentFamily;

    bool isComplete()
    {
        return graphicsFamily.has_value() && presentFamily.has_value();
    }
};

struct SwapChainSupportDetails {
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
};

class HelloTriangleApplication
{
public:
    HelloTriangleApplication()
    {
        initWindow();
        initVulkan();
    }

    ~HelloTriangleApplication()
    {
        cleanup();
    }

    void run()
    {
        mainLoop();
    }

private:
    void initWindow()
    {
        glfwInit();

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
        window = glfwCreateWindow(WIDTH, HEIGHT, TITLE, nullptr, nullptr);
    }

    void initVulkan()
    {
        createInstance();
        createSurface();
        pickPhysicalDevice();
        createLogicalDevice();
        createSwapChain();
        createImageViews();
        createRenderPass();
        createGraphicsPipeline();
        createFramebuffers();
        createCommandPool();
        createCommandBuffers();
    }

    void mainLoop()
    {
        while(!glfwWindowShouldClose(window))
        {
            glfwPollEvents();
        }
    }

    void cleanup()
    {
        vkDestroyCommandPool(device, commandPool, nullptr);

        for (auto framebuffer : swapChainFramebuffers)
        {
            vkDestroyFramebuffer(device, framebuffer, nullptr);
        }

        vkDestroyPipeline(device, graphicsPipeline, nullptr);
        vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
        vkDestroyRenderPass(device, renderPass, nullptr);

        for (auto imageView : swapChainImageViews)
        {
            vkDestroyImageView(device, imageView, nullptr);
        }

        vkDestroySwapchainKHR(device, swapChain, nullptr);
        vkDestroyDevice(device, nullptr);

        if(stringsAreSubsetOfCollection({DEBUG_EXTENSION}, requestedExtensions))
        {
            DestroyDebugReportCallbackEXT(instance, callback);
        }

        vkDestroySurfaceKHR(instance, surface, nullptr);
        vkDestroyInstance(instance, nullptr);
        glfwDestroyWindow(window);
        glfwTerminate();
    }

    void createInstance()
    {
        VkApplicationInfo appInfo = {};
        appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appInfo.pApplicationName = "Hello Triangle";
        appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.pEngineName = "No Engine";
        appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.apiVersion = VK_API_VERSION_1_0;

        std::vector<const char*> requiredExtensions = getRequiredExtensions();
        requiredExtensions.insert(requiredExtensions.end(), requestedExtensions.begin(), requestedExtensions.end());

        std::cout << "EXTENSIONS" << std::endl;
        auto e = enumerateExtensions();
        for(auto l: e)
        {
            std::cout << "\t" << l.extensionName << std::endl;
        }

        std::cout << "checking required extensions:" << std::endl;
        if(isExtensionAvailable(requiredExtensions))
        {
            std::cout << "\tAll needed extensions are available" << std::endl;
        }
        else
        {
            for(const char* extensionNeeded: requiredExtensions)
            {
                if(!isExtensionAvailable(extensionNeeded))
                {
                    std::cout << "\tERROR " << extensionNeeded << " is not available" << std::endl;
                }
            }
            throw std::runtime_error("failed to create instance!");
        }

        std::cout << "checking requested layers:" << std::endl;
        if(isLayerAvailable(validationLayers))
        {
            std::cout << "\tAll " << validationLayers.size() << " requested layers are available" << std::endl;
        }
        else
        {
            for(const char* layerName: validationLayers)
            {
                if(!isLayerAvailable(layerName))
                {
                    std::cout << "\tERROR " << layerName << " is not available" << std::endl;
                }
            }
            throw std::runtime_error("failed to create instance!");
        }


        VkInstanceCreateInfo createInfo    = {};
        createInfo.sType                   = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        createInfo.pApplicationInfo        = &appInfo;
        createInfo.enabledExtensionCount   = requiredExtensions.size();
        createInfo.ppEnabledExtensionNames = requiredExtensions.data();
        createInfo.enabledLayerCount       = validationLayers.size();
        createInfo.ppEnabledLayerNames     = validationLayers.data();

        if( vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS )
        {
            throw std::runtime_error("failed to create instance!");
        }

        if(stringsAreSubsetOfCollection({DEBUG_EXTENSION}, requestedExtensions))
        {
            VkDebugReportCallbackCreateInfoEXT callbackInfo = {};
            callbackInfo.sType   = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
            callbackInfo.flags   = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT;
            callbackInfo.pfnCallback = debugCallback;
            if (CreateDebugReportCallbackEXT(instance, &callbackInfo, nullptr, &callback) != VK_SUCCESS)
            {
                throw std::runtime_error("failed to create debug callbacks");
            }
        }
    }

    bool isExtensionAvailable(const char* extensionName)
    {
        return isExtensionAvailable(&extensionName, 1);
    }

    bool isExtensionAvailable(const char** extensionNames, const int extensionCount)
    {
        return isExtensionAvailable(std::vector<const char*> (extensionNames, extensionNames + extensionCount));
    }
    bool isExtensionAvailable(const std::vector<const char*>& extensionNames)
    {
        return stringsAreSubsetOfCollection<VkExtensionProperties>(extensionNames, enumerateExtensions(), [](const VkExtensionProperties& p){return p.extensionName;});
    }

    bool isDeviceExtensionAvailable(VkPhysicalDevice device, const char* extensionName)
    {
        return isDeviceExtensionAvailable(device, &extensionName, 1);
    }

    bool isDeviceExtensionAvailable(VkPhysicalDevice device, const char** extensionNames, const int extensionCount)
    {
        return isDeviceExtensionAvailable(device, std::vector<const char*> (extensionNames, extensionNames + extensionCount));
    }
    bool isDeviceExtensionAvailable(VkPhysicalDevice device, const std::vector<const char*>& extensionNames)
    {
        return stringsAreSubsetOfCollection<VkExtensionProperties>(extensionNames, enumerateDeviceExtensions(device), [](const VkExtensionProperties& p){return p.extensionName;});
    }

    bool isLayerAvailable(const char* layerName)
    {
        return isLayerAvailable(&layerName, 1);
    }

    bool isLayerAvailable(const char** layerNames, const int layerCount)
    {
        return isLayerAvailable(std::vector<const char*>(layerNames, layerNames + layerCount));
    }

    bool isLayerAvailable(const std::vector<const char*>& layerNames)
    {
        return stringsAreSubsetOfCollection<VkLayerProperties>(layerNames, enumerateLayers(), [](const VkLayerProperties& p){return p.layerName;});
    }

    template<typename T>
    bool stringsAreSubsetOfCollection(const std::vector<const char*>& subset, const std::vector<T>& collection, std::function<const char*(const T&)> accessor = [](const auto &c){return c;})
    {
        bool isSubset = true;
        for (const auto s: subset)
        {
            isSubset &= std::any_of(collection.begin(), collection.end(), [&](const T& c){return std::strcmp(s,accessor(c)) == 0;});
        }
        return isSubset;
    }

    std::vector<VkExtensionProperties> enumerateExtensions()
    {
        uint32_t extensionCount = 0;
        vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);

        std::vector<VkExtensionProperties> extensions(extensionCount);
        vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions.data());

        return extensions;
    }

    std::vector<VkExtensionProperties> enumerateDeviceExtensions(VkPhysicalDevice device)
    {
        uint32_t extensionCount;
        vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

        std::vector<VkExtensionProperties> extensions(extensionCount);
        vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, extensions.data());

        return extensions;
    }

    std::vector<const char*> getRequiredExtensions()
    {
        uint32_t glfwExtensionCount = 0;
        const char** glfwExtensions;
        glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
        return std::vector<const char*>(glfwExtensions, glfwExtensions + glfwExtensionCount);
    }

    std::vector<VkLayerProperties> enumerateLayers()
    {
        uint32_t layerCount = 0;
        vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

        std::vector<VkLayerProperties> availableLayers(layerCount);
        vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

        return availableLayers;
    }

    std::vector<VkPhysicalDevice> enumeratePhysicalDevices()
    {
        uint32_t deviceCount = 0;
        vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
        std::vector<VkPhysicalDevice> devices(deviceCount);
        vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());
        return devices;
    }


    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
        VkDebugReportFlagsEXT                       /*flags*/,
        VkDebugReportObjectTypeEXT                  /*objectType*/,
        uint64_t                                    /*object*/,
        size_t                                      /*location*/,
        int32_t                                     /*messageCode*/,
        const char*                                 pLayerPrefix,
        const char*                                 pMessage,
        void*                                       /*pUserData*/)
    {
        std::cerr << "validation layer " << pLayerPrefix << ": " << pMessage << std::endl;
        return VK_FALSE;
    }

    void pickPhysicalDevice()
    {
        for (const auto& device : enumeratePhysicalDevices())
        {
            if (isDeviceSuitable(device))
            {
                physicalDevice = device;
                break;
            }
        }

        if (physicalDevice == VK_NULL_HANDLE)
        {
            throw std::runtime_error("failed to find a suitable GPU!");
        }

    }

    QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device, VkQueueFlags desiredFlags = VK_QUEUE_GRAPHICS_BIT)
    {
        QueueFamilyIndices indices;

        uint32_t queueFamilyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

        std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

        int i = 0;
        for (const auto& queueFamily : queueFamilies)
        {
            if (queueFamily.queueCount > 0 && queueFamily.queueFlags & desiredFlags)
            {
                indices.graphicsFamily = i;
            }

            VkBool32 presentSupport = false;
            vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);

            if (queueFamily.queueCount > 0 && presentSupport)
            {
                indices.presentFamily = i;
            }

            if (indices.isComplete())
            {
                break;
            }

            i++;
        }
        return indices;
    }

    bool isDeviceSuitable(VkPhysicalDevice device)
    {
        VkPhysicalDeviceProperties deviceProperties;
        VkPhysicalDeviceFeatures deviceFeatures;
        vkGetPhysicalDeviceProperties(device, &deviceProperties);
        vkGetPhysicalDeviceFeatures(device, &deviceFeatures);
        QueueFamilyIndices queueIndices = findQueueFamilies(device);

        bool isSuitable = true;
        isSuitable &= deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU;
        isSuitable &= deviceFeatures.geometryShader;
        isSuitable &= queueIndices.isComplete();
        isSuitable &= isDeviceExtensionAvailable(device, deviceExtensions);

        if(isSuitable)
        {
            SwapChainSupportDetails swapChainSupport = querySwapChainSupport(device);
            isSuitable &= !swapChainSupport.formats.empty();
            isSuitable &= !swapChainSupport.presentModes.empty();
        }

        std::cout << "Checking " << deviceProperties.deviceName << (isSuitable ? " is suitable" : " is NOT suitable") << std::endl;
        // std::cout << "\tExtensions:" << std::endl;
        // std::vector<VkExtensionProperties> extensions = enumerateDeviceExtensions(device);
        // for(const auto& e: extensions)
        // {
        //     std::cout << "\t\t" << e.extensionName << std::endl;
        // }
        //
        return isSuitable;
    }

    void createLogicalDevice()
    {
        QueueFamilyIndices indices = findQueueFamilies(physicalDevice);


        std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
        std::set<uint32_t> uniqueQueueFamilies = {indices.graphicsFamily.value(), indices.presentFamily.value()};

        float queuePriority = 1.0f;
        for (uint32_t queueFamily : uniqueQueueFamilies)
        {
            VkDeviceQueueCreateInfo queueCreateInfo = {};
            queueCreateInfo.sType                   = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queueCreateInfo.queueFamilyIndex        = queueFamily;
            queueCreateInfo.queueCount              = 1;
            queueCreateInfo.pQueuePriorities        = &queuePriority;
            queueCreateInfos.push_back(queueCreateInfo);
        }

        VkPhysicalDeviceFeatures deviceFeatures     = {};

        VkDeviceCreateInfo createInfo               = {};
        createInfo.sType                            = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        createInfo.pQueueCreateInfos                = queueCreateInfos.data();
        createInfo.queueCreateInfoCount             = static_cast<uint32_t>(queueCreateInfos.size());
        createInfo.pEnabledFeatures                 = &deviceFeatures;
        createInfo.enabledExtensionCount            = static_cast<uint32_t>(deviceExtensions.size());
        createInfo.ppEnabledExtensionNames          = deviceExtensions.data();
        createInfo.enabledLayerCount                = static_cast<uint32_t>(validationLayers.size());
        createInfo.ppEnabledLayerNames              = validationLayers.data();

        if (vkCreateDevice(physicalDevice, &createInfo, nullptr, &device) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to create logical device!");
        }

        vkGetDeviceQueue(device, indices.graphicsFamily.value(), 0, &graphicsQueue);
        vkGetDeviceQueue(device, indices.presentFamily.value(), 0, &presentQueue);
    }

    void createSurface()
    {
        if (glfwCreateWindowSurface(instance, window, nullptr, &surface) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to create window surface!");
        }
    }

    SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device)
    {
        SwapChainSupportDetails details;

        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);

        uint32_t formatCount;
        uint32_t presentModeCount;
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);

        details.formats.resize(formatCount);
        details.presentModes.resize(presentModeCount);

        vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, details.formats.data());
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, details.presentModes.data());

        return details;
    }

    VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats)
    {
        if (availableFormats.size() == 1 && availableFormats[0].format == VK_FORMAT_UNDEFINED)
        {
            return {VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR};
        }

        for (const auto& availableFormat : availableFormats)
        {
            if (availableFormat.format == VK_FORMAT_B8G8R8A8_UNORM && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
            {
                return availableFormat;
            }
        }

        return availableFormats[0];
    }

    VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR> availablePresentModes)
    {
        VkPresentModeKHR presentMode = VK_PRESENT_MODE_FIFO_KHR;

        for(const auto& presentationMode: availablePresentModes)
        {
            if(presentationMode==VK_PRESENT_MODE_MAILBOX_KHR)
            {
                return presentationMode;
            }
            if(presentationMode == VK_PRESENT_MODE_IMMEDIATE_KHR)
            {
                presentMode = presentationMode;
            }
        }
        return presentMode;
    }

    VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities)
    {
        if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
        {
            return capabilities.currentExtent;
        }

        VkExtent2D actualExtent = {WIDTH, HEIGHT};

        actualExtent.width  = std::max(capabilities.minImageExtent.width,  std::min(capabilities.maxImageExtent.width,  actualExtent.width));
        actualExtent.height = std::max(capabilities.minImageExtent.height, std::min(capabilities.maxImageExtent.height, actualExtent.height));

        return actualExtent;
    }

    void createSwapChain()
    {
        SwapChainSupportDetails swapChainSupport = querySwapChainSupport(physicalDevice);

        VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
        VkPresentModeKHR   presentMode   = chooseSwapPresentMode(swapChainSupport.presentModes);
        swapChainExtent                  = chooseSwapExtent(swapChainSupport.capabilities);
        swapChainImageFormat             = surfaceFormat.format;

        uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
        if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount)
        {
            imageCount = swapChainSupport.capabilities.maxImageCount;
        }

        VkSwapchainCreateInfoKHR createInfo  = {};
        createInfo.sType                     = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        createInfo.surface                   = surface;
        createInfo.minImageCount             = imageCount;
        createInfo.imageFormat               = surfaceFormat.format;
        createInfo.imageColorSpace           = surfaceFormat.colorSpace;
        createInfo.imageExtent               = swapChainExtent;
        createInfo.imageArrayLayers          = 1;
        createInfo.imageUsage                = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

        QueueFamilyIndices indices = findQueueFamilies(physicalDevice);
        uint32_t queueFamilyIndices[] = {indices.graphicsFamily.value(), indices.presentFamily.value()};

        if (indices.graphicsFamily != indices.presentFamily)
        {
            createInfo.imageSharingMode      = VK_SHARING_MODE_CONCURRENT;
            createInfo.queueFamilyIndexCount = 2;
            createInfo.pQueueFamilyIndices   = queueFamilyIndices;
        }
        else
        {
            createInfo.imageSharingMode      = VK_SHARING_MODE_EXCLUSIVE;
            createInfo.queueFamilyIndexCount = 0; // Optional
            createInfo.pQueueFamilyIndices   = nullptr; // Optional
        }

        createInfo.preTransform              = swapChainSupport.capabilities.currentTransform;
        createInfo.compositeAlpha            = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        createInfo.presentMode               = presentMode;
        createInfo.clipped                   = VK_TRUE;
        createInfo.oldSwapchain              = VK_NULL_HANDLE;

        if (vkCreateSwapchainKHR(device, &createInfo, nullptr, &swapChain) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to create swap chain!");
        }

        vkGetSwapchainImagesKHR(device, swapChain, &imageCount, nullptr);
        swapChainImages.resize(imageCount);
        vkGetSwapchainImagesKHR(device, swapChain, &imageCount, swapChainImages.data());

    }

    void createImageViews()
    {
        swapChainImageViews.resize(swapChainImages.size());

        for (size_t i = 0; i < swapChainImages.size(); i++)
        {
            VkImageViewCreateInfo createInfo           = {};
            createInfo.sType                           = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            createInfo.image                           = swapChainImages[i];
            createInfo.viewType                        = VK_IMAGE_VIEW_TYPE_2D;
            createInfo.format                          = swapChainImageFormat;
            createInfo.components.r                    = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.g                    = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.b                    = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.a                    = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
            createInfo.subresourceRange.baseMipLevel   = 0;
            createInfo.subresourceRange.levelCount     = 1;
            createInfo.subresourceRange.baseArrayLayer = 0;
            createInfo.subresourceRange.layerCount     = 1;

            if (vkCreateImageView(device, &createInfo, nullptr, &swapChainImageViews[i]) != VK_SUCCESS)
            {
                throw std::runtime_error("failed to create image views!");
            }
        }
    }

    void createGraphicsPipeline()
    {
        auto vertShaderCode = readFile("shaders/triangle_vert.spv");
        auto fragShaderCode = readFile("shaders/triangle_frag.spv");
        VkShaderModule vertShaderModule = createShaderModule(vertShaderCode);
        VkShaderModule fragShaderModule = createShaderModule(fragShaderCode);

        VkPipelineShaderStageCreateInfo vertShaderStageInfo      = {};
        vertShaderStageInfo.sType                                = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        vertShaderStageInfo.stage                                = VK_SHADER_STAGE_VERTEX_BIT;
        vertShaderStageInfo.module                               = vertShaderModule;
        vertShaderStageInfo.pName                                = "main";

        VkPipelineShaderStageCreateInfo fragShaderStageInfo      = {};
        fragShaderStageInfo.sType                                = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        fragShaderStageInfo.stage                                = VK_SHADER_STAGE_FRAGMENT_BIT;
        fragShaderStageInfo.module                               = fragShaderModule;
        fragShaderStageInfo.pName                                = "main";

        VkPipelineShaderStageCreateInfo shaderStages[]           = {vertShaderStageInfo, fragShaderStageInfo};

        VkPipelineVertexInputStateCreateInfo vertexInputInfo     = {};
        vertexInputInfo.sType                                    = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        vertexInputInfo.vertexBindingDescriptionCount            = 0;
        vertexInputInfo.pVertexBindingDescriptions               = nullptr; // Optional
        vertexInputInfo.vertexAttributeDescriptionCount          = 0;
        vertexInputInfo.pVertexAttributeDescriptions             = nullptr; // Optional

        VkPipelineInputAssemblyStateCreateInfo inputAssembly     = {};
        inputAssembly.sType                                      = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        inputAssembly.topology                                   = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        inputAssembly.primitiveRestartEnable                     = VK_FALSE;

        VkViewport viewport                                      = {};
        viewport.x                                               = 0.0f;
        viewport.y                                               = 0.0f;
        viewport.width                                           = (float) swapChainExtent.width;
        viewport.height                                          = (float) swapChainExtent.height;
        viewport.minDepth                                        = 0.0f;
        viewport.maxDepth                                        = 1.0f;

        VkRect2D scissor                                         = {};
        scissor.offset                                           = {0, 0};
        scissor.extent                                           = swapChainExtent;

        VkPipelineViewportStateCreateInfo viewportState          = {};
        viewportState.sType                                      = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewportState.viewportCount                              = 1;
        viewportState.pViewports                                 = &viewport;
        viewportState.scissorCount                               = 1;
        viewportState.pScissors                                  = &scissor;

        VkPipelineRasterizationStateCreateInfo rasterizer        = {};
        rasterizer.sType                                         = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterizer.depthClampEnable                              = VK_FALSE;
        rasterizer.rasterizerDiscardEnable                       = VK_FALSE;
        rasterizer.polygonMode                                   = VK_POLYGON_MODE_FILL;
        rasterizer.lineWidth                                     = 1.0f;
        rasterizer.cullMode                                      = VK_CULL_MODE_BACK_BIT;
        rasterizer.frontFace                                     = VK_FRONT_FACE_CLOCKWISE;
        rasterizer.depthBiasEnable                               = VK_FALSE;
        rasterizer.depthBiasConstantFactor                       = 0.0f; // Optional
        rasterizer.depthBiasClamp                                = 0.0f; // Optional
        rasterizer.depthBiasSlopeFactor                          = 0.0f; // Optional

        VkPipelineMultisampleStateCreateInfo multisampling       = {};
        multisampling.sType                                      = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multisampling.sampleShadingEnable                        = VK_FALSE;
        multisampling.rasterizationSamples                       = VK_SAMPLE_COUNT_1_BIT;
        multisampling.minSampleShading                           = 1.0f; // Optional
        multisampling.pSampleMask                                = nullptr; // Optional
        multisampling.alphaToCoverageEnable                      = VK_FALSE; // Optional
        multisampling.alphaToOneEnable                           = VK_FALSE; // Optional

        VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
        colorBlendAttachment.colorWriteMask                      = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        colorBlendAttachment.blendEnable                         = VK_FALSE;
        colorBlendAttachment.srcColorBlendFactor                 = VK_BLEND_FACTOR_ONE; // Optional
        colorBlendAttachment.dstColorBlendFactor                 = VK_BLEND_FACTOR_ZERO; // Optional
        colorBlendAttachment.colorBlendOp                        = VK_BLEND_OP_ADD; // Optional
        colorBlendAttachment.srcAlphaBlendFactor                 = VK_BLEND_FACTOR_ONE; // Optional
        colorBlendAttachment.dstAlphaBlendFactor                 = VK_BLEND_FACTOR_ZERO; // Optional
        colorBlendAttachment.alphaBlendOp                        = VK_BLEND_OP_ADD; // Optional

        VkPipelineColorBlendStateCreateInfo colorBlending        = {};
        colorBlending.sType                                      = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        colorBlending.logicOpEnable                              = VK_FALSE;
        colorBlending.logicOp                                    = VK_LOGIC_OP_COPY; // Optional
        colorBlending.attachmentCount                            = 1;
        colorBlending.pAttachments                               = &colorBlendAttachment;
        colorBlending.blendConstants[0]                          = 0.0f; // Optional
        colorBlending.blendConstants[1]                          = 0.0f; // Optional
        colorBlending.blendConstants[2]                          = 0.0f; // Optional
        colorBlending.blendConstants[3]                          = 0.0f; // Optional

        VkPipelineLayoutCreateInfo pipelineLayoutInfo            = {};
        pipelineLayoutInfo.sType                                 = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.setLayoutCount                        = 0; // Optional
        pipelineLayoutInfo.pSetLayouts                           = nullptr; // Optional
        pipelineLayoutInfo.pushConstantRangeCount                = 0; // Optional
        pipelineLayoutInfo.pPushConstantRanges                   = nullptr; // Optional

        if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to create pipeline layout!");
        }

        VkGraphicsPipelineCreateInfo pipelineInfo                = {};
        pipelineInfo.sType                                       = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipelineInfo.stageCount                                  = 2;
        pipelineInfo.pStages                                     = shaderStages;
        pipelineInfo.pVertexInputState                           = &vertexInputInfo;
        pipelineInfo.pInputAssemblyState                         = &inputAssembly;
        pipelineInfo.pViewportState                              = &viewportState;
        pipelineInfo.pRasterizationState                         = &rasterizer;
        pipelineInfo.pMultisampleState                           = &multisampling;
        pipelineInfo.pDepthStencilState                          = nullptr; // Optional
        pipelineInfo.pColorBlendState                            = &colorBlending;
        pipelineInfo.pDynamicState                               = nullptr; // Optional
        pipelineInfo.layout                                      = pipelineLayout;
        pipelineInfo.renderPass                                  = renderPass;
        pipelineInfo.subpass                                     = 0;
        pipelineInfo.basePipelineHandle                          = VK_NULL_HANDLE; // Optional
        pipelineInfo.basePipelineIndex                           = -1; // Optional

        if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphicsPipeline) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to create graphics pipeline!");
        }


        vkDestroyShaderModule(device, fragShaderModule, nullptr);
        vkDestroyShaderModule(device, vertShaderModule, nullptr);
    }


    static std::vector<char> readFile(const std::string& filename)
    {
        std::ifstream file(filename, std::ios::ate | std::ios::binary);

        if (!file.is_open())
        {
            throw std::runtime_error("failed to open file!");
        }

        const size_t fileSize = static_cast<size_t>(file.tellg());
        std::vector<char> buffer(fileSize);
        file.seekg(0);
        file.read(buffer.data(), fileSize);
        file.close();

        return buffer;
    }

    VkShaderModule createShaderModule(const std::vector<char>& code)
    {
        VkShaderModuleCreateInfo createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        createInfo.codeSize = code.size();
        createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

        VkShaderModule shaderModule;
        if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to create shader module!");
        }
        return shaderModule;
    }

    void createRenderPass()
    {
        VkAttachmentDescription colorAttachment  = {};
        colorAttachment.format                   = swapChainImageFormat;
        colorAttachment.samples                  = VK_SAMPLE_COUNT_1_BIT;
        colorAttachment.loadOp                   = VK_ATTACHMENT_LOAD_OP_CLEAR;
        colorAttachment.storeOp                  = VK_ATTACHMENT_STORE_OP_STORE;
        colorAttachment.stencilLoadOp            = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        colorAttachment.stencilStoreOp           = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        colorAttachment.initialLayout            = VK_IMAGE_LAYOUT_UNDEFINED;
        colorAttachment.finalLayout              = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

        VkAttachmentReference colorAttachmentRef = {};
        colorAttachmentRef.attachment            = 0;
        colorAttachmentRef.layout                = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkSubpassDescription subpass             = {};
        subpass.pipelineBindPoint                = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount             = 1;
        subpass.pColorAttachments                = &colorAttachmentRef;

        VkRenderPassCreateInfo renderPassInfo    = {};
        renderPassInfo.sType                     = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        renderPassInfo.attachmentCount           = 1;
        renderPassInfo.pAttachments              = &colorAttachment;
        renderPassInfo.subpassCount              = 1;
        renderPassInfo.pSubpasses                = &subpass;

        if (vkCreateRenderPass(device, &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to create render pass!");
        }
    }

    void createFramebuffers()
    {
        swapChainFramebuffers.resize(swapChainImageViews.size());
        for (size_t i = 0; i < swapChainImageViews.size(); i++)
        {
            VkImageView attachments[] =
            {
                swapChainImageViews[i]
            };

            VkFramebufferCreateInfo framebufferInfo = {};
            framebufferInfo.sType                   = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            framebufferInfo.renderPass              = renderPass;
            framebufferInfo.attachmentCount         = 1;
            framebufferInfo.pAttachments            = attachments;
            framebufferInfo.width                   = swapChainExtent.width;
            framebufferInfo.height                  = swapChainExtent.height;
            framebufferInfo.layers                  = 1;

            if (vkCreateFramebuffer(device, &framebufferInfo, nullptr, &swapChainFramebuffers[i]) != VK_SUCCESS)
            {
                throw std::runtime_error("failed to create framebuffer!");
            }
        }
    }

    void createCommandPool()
    {
        QueueFamilyIndices queueFamilyIndices = findQueueFamilies(physicalDevice);

        VkCommandPoolCreateInfo poolInfo = {};
        poolInfo.sType                   = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        poolInfo.queueFamilyIndex        = queueFamilyIndices.graphicsFamily.value();
        poolInfo.flags                   = 0; // Optional

        if (vkCreateCommandPool(device, &poolInfo, nullptr, &commandPool) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to create command pool!");
        }
    }

    void createCommandBuffers()
    {
        commandBuffers.resize(swapChainFramebuffers.size());
        VkCommandBufferAllocateInfo allocInfo = {};
        allocInfo.sType                       = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.commandPool                 = commandPool;
        allocInfo.level                       = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandBufferCount          = static_cast<uint32_t>(commandBuffers.size());

        if (vkAllocateCommandBuffers(device, &allocInfo, commandBuffers.data()) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to allocate command buffers!");
        }

        for (size_t i = 0; i < commandBuffers.size(); i++)
        {
            VkCommandBufferBeginInfo beginInfo = {};
            beginInfo.sType                    = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
            beginInfo.flags                    = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
            beginInfo.pInheritanceInfo         = nullptr; // Optional

            if (vkBeginCommandBuffer(commandBuffers[i], &beginInfo) != VK_SUCCESS)
            {
                throw std::runtime_error("failed to begin recording command buffer!");
            }

            VkClearValue clearColor              = {0.0f, 0.0f, 0.0f, 1.0f};

            VkRenderPassBeginInfo renderPassInfo = {};
            renderPassInfo.sType                 = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
            renderPassInfo.renderPass            = renderPass;
            renderPassInfo.framebuffer           = swapChainFramebuffers[i];
            renderPassInfo.renderArea.offset     = {0, 0};
            renderPassInfo.renderArea.extent     = swapChainExtent;
            renderPassInfo.clearValueCount       = 1;
            renderPassInfo.pClearValues          = &clearColor;

            vkCmdBeginRenderPass(commandBuffers[i], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
            vkCmdBindPipeline(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);
            vkCmdDraw(commandBuffers[i], 3, 1, 0, 0);
            vkCmdEndRenderPass(commandBuffers[i]);

            if (vkEndCommandBuffer(commandBuffers[i]) != VK_SUCCESS)
            {
                throw std::runtime_error("failed to record command buffer!");
            }
        }
    }


    GLFWwindow*                  window         = nullptr;
    VkInstance                   instance;
    VkDebugReportCallbackEXT     callback;
    VkSurfaceKHR                 surface;
    VkPhysicalDevice             physicalDevice = VK_NULL_HANDLE;
    VkDevice                     device;
    VkQueue                      graphicsQueue;
    VkQueue                      presentQueue;
    VkSwapchainKHR               swapChain;
    std::vector<VkImage>         swapChainImages;
    VkFormat                     swapChainImageFormat;
    VkExtent2D                   swapChainExtent;
    std::vector<VkImageView>     swapChainImageViews;
    VkRenderPass                 renderPass;
    VkPipelineLayout             pipelineLayout;
    VkPipeline                   graphicsPipeline;
    std::vector<VkFramebuffer>   swapChainFramebuffers;
    VkCommandPool                commandPool;
    std::vector<VkCommandBuffer> commandBuffers;
};

int main()
{
    HelloTriangleApplication app;

    try
    {
        app.run();
    }
    catch (const std::exception& e)
    {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
