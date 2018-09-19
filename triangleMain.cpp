#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <iostream>
#include <stdexcept>
#include <functional>
#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <vector>


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

    std::vector<VkLayerProperties> enumerateLayers()
    {
        uint32_t layerCount = 0;
        vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

        std::vector<VkLayerProperties> availableLayers(layerCount);
        vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

        return availableLayers;
    }

    std::vector<const char*> getRequiredExtensions()
    {
        uint32_t glfwExtensionCount = 0;
        const char** glfwExtensions;
        glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
        return std::vector<const char*>(glfwExtensions, glfwExtensions + glfwExtensionCount);
    }

    GLFWwindow* window = nullptr;
    VkInstance  instance;
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
