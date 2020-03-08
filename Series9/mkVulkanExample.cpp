#include <iostream>
#include <stdexcept>
#include <functional>
#include <cstdlib>
#include <util_init.hpp>
#include <set>

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
			searchQueue();
			//MK: (코드 1-2) Create Logical Device 함수 호출
			createLogicalDevice();
		}

		void mainLoop(){
		}

		void cleanup(){
			//MK: (코드 1- 9) Logical Deivce Destory 함수 호출
			vkDestroyDevice(device, nullptr);
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

		void searchQueue(){
			LOGI("MK: searchQueue Function");

			uint32_t queueCount = 0;
			vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueCount, nullptr);
			assert(queueCount > 0);
			std::vector<VkQueueFamilyProperties> queueFamilies(queueCount);
			vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueCount, queueFamilies.data());

			LOGI("MK: Queue Family Count - %d", queueCount);

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

		//MK: (코드 1-1) Logical Device를 생성하기 위한 코드
		void createLogicalDevice(){
			
			//MK: (코드 1-4) Unique Queue 번호를 찾음
			std::set<uint32_t> uniqueQueueFamilies = {graphicQueueFamilyIndex, presentQueueFamilyIndex};

			std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;

			//MK: (코드 1-5) queueCreateInfo Struct 값 입력. Qeuue Family가 여러개의 경우 여러개의 queueCreateInfo를 생성함
			float queuePriority = 1.0f;
			for(uint32_t queueFamily : uniqueQueueFamilies){
				VkDeviceQueueCreateInfo queueCreateInfo = {};
				queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
				queueCreateInfo.pNext = NULL;
				queueCreateInfo.flags = 0;
				queueCreateInfo.queueFamilyIndex = queueFamily;
				queueCreateInfo.queueCount = 1;
				queueCreateInfo.pQueuePriorities = &queuePriority;
				queueCreateInfos.push_back(queueCreateInfo);
			}

			//MK: (코드 1-6) 필요한 Device Feature, Extension을 추가함
			VkPhysicalDeviceFeatures deviceFeatures = {};
			std::vector<const char *> deviceExtensions;
			deviceExtensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);

			//MK: (코드 1-7) VkDeviceCreateInfo Struct에 필요한 값을 입력함
			VkDeviceCreateInfo createInfo = {};
			createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
			createInfo.pNext = NULL;
			createInfo.flags = 0;
			createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
			createInfo.pQueueCreateInfos = queueCreateInfos.data();
			//createInfo.enabledLayerCount
			//createInfo.ppEnabledLayerNames
			createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
			createInfo.ppEnabledExtensionNames = deviceExtensions.data();
			createInfo.pEnabledFeatures = &deviceFeatures;

			//MK: (코드 1-8) Logical Device를 생성함
			VkResult result = vkCreateDevice(physicalDevice, &createInfo, NULL, &device);

			assert(result == VK_SUCCESS);
			LOGI("MK: Successfully Create Logical Device (If not, should not see this print)");

			//MK: (코드 1-11) 생성된 Queue를 저장하기 위한 코드
			vkGetDeviceQueue(device, graphicQueueFamilyIndex, 0, &graphicsQueue);
			vkGetDeviceQueue(device, presentQueueFamilyIndex, 0, &presentQueue);
		}

		VkInstance instance;

		VkSurfaceKHR surface;
		PFN_vkCreateAndroidSurfaceKHR fpCreateAndroidSurfaceKHR;

		VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;

		uint32_t graphicQueueFamilyIndex = 0;
		uint32_t presentQueueFamilyIndex = 0;

		//MK: (코드 1-3) Logical Device 변수 추가
		VkDevice device; 

		//MK: (코드 1-10) Queue를 저장하기 위한 변수 추가
		VkQueue graphicsQueue;
		VkQueue presentQueue;
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
