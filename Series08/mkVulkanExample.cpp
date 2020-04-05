#include <iostream>
#include <stdexcept>
#include <functional>
#include <cstdlib>
#include <util_init.hpp>

#define APP_SHORT_NAME "mkVulkanExample"

#define MK_GET_INSTANCE_PROC_ADDR(inst, entrypoint)                               			\
{                                                                          					\
	fp##entrypoint = (PFN_vk##entrypoint)vkGetInstanceProcAddr(instance, "vk" #entrypoint); \
	if (fp##entrypoint == NULL) {                                     						\
		std::cout << "vkGetDeviceProcAddr failed to find vk" #entrypoint;  					\
		exit(-1);                                                          					\
	}                                                                      					\
}


class mkTriangle{
	public:
		void run(){
			LOGI("MK: mkTriangle-->run()");
			initVulkan();
			mainLoop();
			cleanup();
		}

	private:
		void initVulkan(){
			createInstance();
			createSurface();
			pickPhysicalDevice();
			//MK: (코드 1-2) Graphic, Present Queue Index를 찾기 위한 함수 호출
			searchQueue();
		}

		void mainLoop(){
		}

		void cleanup(){
			vkDestroySurfaceKHR(instance, surface, NULL);
			vkDestroyInstance(instance, NULL);
		}

		void createInstance(){

			LOGI("MK: Create VkApplicationInfo");
			VkApplicationInfo appInfo = {};
			appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
			appInfo.pNext = NULL;
			appInfo.pApplicationName = APP_SHORT_NAME;
			appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
			appInfo.pEngineName = "No Engine";
			appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
			appInfo.apiVersion = VK_API_VERSION_1_0;

			uint32_t extensionCount = 0;
			vkEnumerateInstanceExtensionProperties(NULL, &extensionCount, NULL);
			std::vector<VkExtensionProperties> extensions(extensionCount);
			vkEnumerateInstanceExtensionProperties(NULL, &extensionCount, extensions.data());
			LOGI("MK: Instance Extensions Count - %d", extensionCount);
			std::vector<const char *> instanceExtensions;
			for (const auto& extension : extensions){
				LOGI("MK: Instance Extension - %s", extension.extensionName);
				//MK: 필요 Extnesion은 String으로 보내야 함
				instanceExtensions.push_back(extension.extensionName);
			}

			uint32_t layerCount = 0;
			vkEnumerateInstanceLayerProperties(&layerCount, NULL);
			std::vector<VkLayerProperties> layers(layerCount);
			vkEnumerateInstanceLayerProperties(&layerCount, layers.data());

			LOGI("MK: Instance Layer Count - %d", layerCount);
			std::vector<const char *> instanceLayers;
			for(const auto& layer : layers){
				LOGI("MK: Instance Layer - %s", layer.layerName);
				//MK: 필요 layer은 String으로 보내야함
				instanceLayers.push_back(layer.layerName);
			}

			LOGI("MK: Create VkInstanceCreateInfo");
			VkInstanceCreateInfo createInfo = {};
			createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
			createInfo.pNext = NULL;
			createInfo.flags = 0;
			createInfo.pApplicationInfo = &appInfo;
			createInfo.enabledLayerCount = layerCount;
			createInfo.ppEnabledLayerNames = layerCount ? instanceLayers.data() : NULL;
			createInfo.enabledExtensionCount = extensionCount;
			createInfo.ppEnabledExtensionNames = extensionCount ? instanceExtensions.data() : NULL;

			VkResult result = vkCreateInstance(&createInfo, NULL, &instance);
			
			assert(result == VK_SUCCESS);
			LOGI("MK: Successfully Create Instance (If not, should not see this print)");

		}

		void createSurface(){
			
			LOGI("MK: createSurface Function");
			VkAndroidSurfaceCreateInfoKHR createInfo = {};
			createInfo.sType = VK_STRUCTURE_TYPE_ANDROID_SURFACE_CREATE_INFO_KHR;
			createInfo.pNext = NULL;
			createInfo.flags = 0;
			createInfo.window = AndroidGetApplicationWindow();

			MK_GET_INSTANCE_PROC_ADDR(instance, CreateAndroidSurfaceKHR);
			
			VkResult result = fpCreateAndroidSurfaceKHR(instance, &createInfo, NULL, &surface);

			assert(result == VK_SUCCESS);
			LOGI("MK: Successfully Create Surface (If not, should not see this print)");
		}

		void pickPhysicalDevice(){
			LOGI("MK: pickPhysicalDevice Function");

			uint32_t deviceCount = 0;
			vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
			assert(deviceCount != 0);

			LOGI("MK: Physical Device Count - %d", deviceCount);

			std::vector<VkPhysicalDevice> devices(deviceCount);
			vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

			for(const auto &device : devices){
				printPhysicalDeviceInfo(device);
			}

			physicalDevice = devices[0];

			assert(physicalDevice != VK_NULL_HANDLE);
			LOGI("MK: Successfully Select Physical Device (If not, should not see this print)");
		}

		void printPhysicalDeviceInfo(VkPhysicalDevice device){
			VkPhysicalDeviceProperties deviceProperties;
			VkPhysicalDeviceFeatures deviceFeatures;
			vkGetPhysicalDeviceProperties(device, &deviceProperties);
			vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

			LOGI("MK: Physical Device Name - %s", deviceProperties.deviceName);
			LOGI("MK: Physical Device - geometryShader (%d)", deviceFeatures.geometryShader);
			LOGI("MK: Physical Device - shaderInt64 (%d)", deviceFeatures.shaderInt64);
		}

		//MK: (코드 1-1) Graphic, Present Queue를 찾기 위한 함수
		void searchQueue(){
			LOGI("MK: searchQueue Function");

			//MK: (코드 1-4) GPU가 가지고 있는 모든 Framily Queue 정보를 파악하는 코드
			uint32_t queueCount = 0;
			vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueCount, nullptr);
			assert(queueCount > 0);
			std::vector<VkQueueFamilyProperties> queueFamilies(queueCount);
			vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueCount, queueFamilies.data());

			LOGI("MK: Queue Family Count - %d", queueCount);

			//MK: (코드 1-5) Graphics Queue와 Present를 Support하는 Queue Family를 찾는 코드
			bool graphicsFound = false;
			bool presentFound = false;
			for(int i = 0; i < queueCount; i++){
				LOGI("MK: %d QueueFamily has a total of %d", i, queueFamilies[i].queueCount);
				if(queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT){
					graphicQueueFamilyIndex = i;
					graphicsFound = true;
				}

				VkBool32 presentSupport = false;
				vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, i, surface, &presentSupport);

				if(presentSupport){
					presentQueueFamilyIndex = i;
					presentFound = true;
				}

				if(presentFound && graphicsFound){
					break;
				}
			}
			assert(graphicsFound);
			assert(presentFound);

			LOGI("MK: Found Graphic Queue Family = %d", graphicQueueFamilyIndex);
			LOGI("MK: Found Queue Family with Present Support = %d", presentQueueFamilyIndex);

			LOGI("MK: Found Graphics Queue with Present Support (If not, should not see this print)");
		}

		VkInstance instance;

		VkSurfaceKHR surface;
		PFN_vkCreateAndroidSurfaceKHR fpCreateAndroidSurfaceKHR;

		VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;

		//MK: (코드 1-3) graphic, present queue index를 저장하기 위한 변수
		uint32_t graphicQueueFamilyIndex = 0;
		uint32_t presentQueueFamilyIndex = 0;
};

int sample_main(int argc, char *argv[]) {
	
	if(!InitVulkan()){
		LOGE("MK: Failed to Initialize Vulkan APIs");
		return EXIT_FAILURE;
	}
		
	mkTriangle mkApp;

	try{
		mkApp.run();
	} catch (const std::exception &e){
		std::cerr << e.what() << std::endl;
		LOGE("MK: Failed to Run Application");
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}
