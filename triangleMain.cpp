#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <iostream>
#include <stdexcept>
#include <functional>
#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <vector>
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

    bool isComplete()
    {
        return graphicsFamily.has_value();
    }
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

        std::cout << "Checking " << deviceProperties.deviceName << (isSuitable ? " is suitable" : " is NOT suitable") << std::endl;
        return isSuitable;
    }

    void createLogicalDevice()
    {
        QueueFamilyIndices indices = findQueueFamilies(physicalDevice);

        float queuePriority                     = 1.0f;

        VkDeviceQueueCreateInfo queueCreateInfo = {};
        queueCreateInfo.sType                   = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex        = indices.graphicsFamily.value();
        queueCreateInfo.queueCount              = 1;
        queueCreateInfo.pQueuePriorities        = &queuePriority;

        VkPhysicalDeviceFeatures deviceFeatures = {};

        VkDeviceCreateInfo createInfo           = {};
        createInfo.sType                        = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        createInfo.pQueueCreateInfos            = &queueCreateInfo;
        createInfo.queueCreateInfoCount         = 1;
        createInfo.pEnabledFeatures             = &deviceFeatures;
        createInfo.enabledExtensionCount        = 0;
        createInfo.enabledLayerCount            = static_cast<uint32_t>(validationLayers.size());
        createInfo.ppEnabledLayerNames          = validationLayers.data();

        if (vkCreateDevice(physicalDevice, &createInfo, nullptr, &device) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to create logical device!");
        }

        vkGetDeviceQueue(device, indices.graphicsFamily.value(), 0, &graphicsQueue);
    }

    void createSurface()
    {
        if (glfwCreateWindowSurface(instance, window, nullptr, &surface) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to create window surface!");
        }
    }

    GLFWwindow*              window         = nullptr;
    VkInstance               instance;
    VkDebugReportCallbackEXT callback;
    VkSurfaceKHR             surface;
    VkPhysicalDevice         physicalDevice = VK_NULL_HANDLE;
    VkDevice                 device;
    VkQueue                  graphicsQueue;
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
