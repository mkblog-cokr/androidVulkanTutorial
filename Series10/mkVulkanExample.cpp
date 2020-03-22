#include <iostream>
#include <stdexcept>
#include <functional>
#include <cstdlib>
#include <util_init.hpp>
#include <set>

#define APP_SHORT_NAME "mkVulkanExample"

#define MK_GET_INSTANCE_PROC_ADDR(inst, entrypoint)												\
{																								\
	fp##entrypoint = (PFN_vk##entrypoint)vkGetInstanceProcAddr(instance, "vk" #entrypoint);		\
	if (fp##entrypoint == NULL) {																\
		std::cout << "vkGetDeviceProcAddr failed to find vk" #entrypoint;						\
		exit(-1);																				\
	}																							\
}


class mkTriangle{
	public:
		void run(){
			LOGI("MK: mkTriangle-->run Function");
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
			createLogicalDevice();

			//MK: (코드 1-2) createSwapChain() 함수 호출
			createSwapChain();
		}

		void mainLoop(){
		}

		void cleanup(){
			LOGI("MK: cleanup Function");

			//MK: (코드 1-14) Swap Chain 제거 함수 호출
			vkDestroySwapchainKHR(device, swapChain, NULL);

			vkDestroyDevice(device, NULL);
			vkDestroySurfaceKHR(instance, surface, NULL);
			vkDestroyInstance(instance, NULL);
		}

		void createInstance(){

			LOGI("MK: createInstance() Function");
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

			//LOGI("MK: Create VkInstanceCreateInfo");
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
			vkEnumeratePhysicalDevices(instance, &deviceCount, NULL);
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
			vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueCount, NULL);
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

		void createLogicalDevice(){

			LOGI("MK: createLogicalDevice Function");
			
			std::set<uint32_t> uniqueQueueFamilies = {graphicQueueFamilyIndex, presentQueueFamilyIndex};

			std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;

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

			//MK: (코드 1-3) Swap Chain Extension 추가
			//MK: create logical device 설명 때 추가함
			VkPhysicalDeviceFeatures deviceFeatures = {};
			std::vector<const char *> deviceExtensions;
			deviceExtensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);

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

			VkResult result = vkCreateDevice(physicalDevice, &createInfo, NULL, &device);

			assert(result == VK_SUCCESS);
			LOGI("MK: Successfully Create Logical Device (If not, should not see this print)");

			vkGetDeviceQueue(device, graphicQueueFamilyIndex, 0, &graphicsQueue);
			vkGetDeviceQueue(device, presentQueueFamilyIndex, 0, &presentQueue);
		}

		//MK: (코드 1-1) createSwapChain() 함수 생성
		void createSwapChain(){
			LOGI("MK: (Begin) createSwapChain Function");
			
			//MK: (코드 1-5) Surface Capability 로딩  코드
			VkResult result = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &surfaceCapabilities);
			assert(result == VK_SUCCESS);
			LOGI("\tMK: Successfully Getting Surface Capability");
			LOGI("\t\tMK: minImageCount - %d, maxImageCount - %d", surfaceCapabilities.minImageCount, surfaceCapabilities.maxImageCount);

			//MK: (코드 1-6) Surface Format 로딩 코드
			uint32_t formatCount;
			result = vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, NULL);
			assert(result == VK_SUCCESS);
			assert(formatCount > 0);
			surfaceFormats.resize(formatCount);
			result = vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, surfaceFormats.data());
			LOGI("\tMK: Successfully Getting Surface Formats (Total Format Count - %d)", formatCount);
			for(int i = 0; i < formatCount; i++){
				LOGI("\t\tMK: Surface Format - %d, Color Space - %d", surfaceFormats[i].format, surfaceFormats[i].colorSpace);
			}

			//MK: (코드 1-7) Presentation Mode 로딩 코드
			uint32_t presentModeCount;
			result = vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, NULL);
			assert(result == VK_SUCCESS);
			assert(presentModeCount > 0);
			presentModes.resize(presentModeCount);
			result = vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, presentModes.data());
			LOGI("\tMK: Successfully Getting Presentation Modes (Total Presentation Mode Count - %d)", presentModeCount);
			for(int i = 0; i < presentModeCount; i++){
				LOGI("\t\tMK: Present Mode - %d", presentModes[i]);
			}

			//MK: (코드 1-9) Surface Format 선택하는 코드
			bool found = false;
			for(int i = 0; i < formatCount; i++){
				if(surfaceFormats[i].format == VK_FORMAT_R8G8B8A8_UNORM){
					LOGI("\tMK: Found VK_FORMAT_R8G8B8A8_UNORM (ID - %d)", i);
					selectedSurfaceFormat = surfaceFormats[i];
					found = true;
					break;
				}
			}

			if(!found){
				selectedSurfaceFormat = surfaceFormats[0];
				LOGI("\tMK: Select First Available Surface Format");
			}


			//MK: (코드 1-10) Presentation Mode 선택 코드
			found = false;
			for(int i = 0; i < presentModeCount; i++){
				if(presentModes[i] == VK_PRESENT_MODE_MAILBOX_KHR){
					LOGI("\tMK: Found VK_PRESENT_MODE_MAILBOX_KHR (ID - %d)", i);
					selectedPresentMode = VK_PRESENT_MODE_MAILBOX_KHR;
					found = true;
					break;
				}
			}

			if(!found){
				LOGI("\tMK: Default Prsent Mode: VK_PRESENT_MODE_FIFO_KHR");
				selectedPresentMode = VK_PRESENT_MODE_FIFO_KHR;
			}

			//MK: (코드 1-11) Swap Extent 선택 코드
			int width, height;
			AndroidGetWindowSize(&width, &height);
			if(surfaceCapabilities.currentExtent.width == UINT32_MAX){
				selectedExtent.width = width;
				selectedExtent.height = height;
				//MK: Width Min/Max 안에 값이 존재하는지 확인
				if(selectedExtent.width < surfaceCapabilities.minImageExtent.width){
					selectedExtent.width = surfaceCapabilities.minImageExtent.width;
				}
				else if(selectedExtent.width > surfaceCapabilities.maxImageExtent.width){
					selectedExtent.width = surfaceCapabilities.maxImageExtent.width;
				}
				//MK: Height Min/Max 안에 값이 존재하는지 확인
				if(selectedExtent.height < surfaceCapabilities.minImageExtent.height){
					selectedExtent.height = surfaceCapabilities.minImageExtent.height;
				}
				else if(selectedExtent.height > surfaceCapabilities.maxImageExtent.height){
					selectedExtent.height = surfaceCapabilities.maxImageExtent.height;
				}
			}
			else{
				//MK: Swap Exnet가 미리 정의된 경우 해당 Setting을 사용함 
				selectedExtent = surfaceCapabilities.currentExtent;
			}
			LOGI("\tMK: Selected Swap Extent Size - (%d x %d)", selectedExtent.width, selectedExtent.height);

			//MK: (코드 1-12) Image 개수 선택 코드
			selectedImageCount = surfaceCapabilities.minImageCount + 1;
			if(surfaceCapabilities.maxImageCount > 0 && selectedImageCount > surfaceCapabilities.maxImageCount){
				selectedImageCount = surfaceCapabilities.maxImageCount;
			}
			LOGI("\tMK: Selected Image Count - %d", selectedImageCount);

			//MK: (코드 1-14) Swap Chain 생성 코드
			VkSwapchainCreateInfoKHR createInfo = {};
			createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
			createInfo.pNext = NULL;
			createInfo.flags = 0;
			createInfo.surface = surface;
			createInfo.minImageCount = selectedImageCount;
			createInfo.imageFormat = selectedSurfaceFormat.format;
			createInfo.imageColorSpace = selectedSurfaceFormat.colorSpace;
			createInfo.imageExtent = selectedExtent;
			createInfo.imageArrayLayers = 1;
			createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
			uint32_t queueFamilyIndices[] = {graphicQueueFamilyIndex, presentQueueFamilyIndex};
			if(graphicQueueFamilyIndex != presentQueueFamilyIndex){
				createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
				createInfo.queueFamilyIndexCount = 2;
				createInfo.pQueueFamilyIndices = queueFamilyIndices;
			}
			else{
				createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
				createInfo.queueFamilyIndexCount = 0;
				createInfo.pQueueFamilyIndices = NULL;
			}
			createInfo.preTransform = surfaceCapabilities.currentTransform;
			createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
			createInfo.presentMode = selectedPresentMode;
			createInfo.clipped = VK_TRUE;
			createInfo.oldSwapchain = VK_NULL_HANDLE;

			result = vkCreateSwapchainKHR(device, &createInfo, NULL, &swapChain);
			assert(result == VK_SUCCESS);
			LOGI("MK: Successfully Create Swap Chain (If not, should not see this print)");

			//MK: (코드 1-16) Swap Chain Image 저장 
			uint32_t imageCount;
			result = vkGetSwapchainImagesKHR(device, swapChain, &imageCount, NULL);
			assert(result == VK_SUCCESS);
			swapChainImages.resize(imageCount);
			result = vkGetSwapchainImagesKHR(device, swapChain, &imageCount, swapChainImages.data());
			assert(result == VK_SUCCESS);
			LOGI("MK: Successfully Retrieveing %d Swap Chain Images (If not, should not see this print)", imageCount);

			LOGI("MK: (End) createSwapChain Function");
		}

		VkInstance instance;

		VkSurfaceKHR surface;
		PFN_vkCreateAndroidSurfaceKHR fpCreateAndroidSurfaceKHR;

		VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;

		uint32_t graphicQueueFamilyIndex = 0;
		uint32_t presentQueueFamilyIndex = 0;

		VkDevice device; 

		VkQueue graphicsQueue;
		VkQueue presentQueue;

		//MK: (코드 1-4) Basic Surface Capability, Surface Formats, Present Mode를 확인 하기 위해서 변수 추가
		VkSurfaceCapabilitiesKHR surfaceCapabilities;
		std::vector<VkSurfaceFormatKHR> surfaceFormats;
		std::vector<VkPresentModeKHR> presentModes;

		//MK: (코드 1-8) Surface Format, Presentation Mode, Swap Extend, Image 개수를 선택하기 위해서 변수 추가
		VkSurfaceFormatKHR selectedSurfaceFormat;
		VkPresentModeKHR selectedPresentMode;
		VkExtent2D selectedExtent;
		uint32_t selectedImageCount;

		//MK: (코드 1-13) Swap Chain 변수 추가
		VkSwapchainKHR swapChain;

		//MK: (코드 1-15) Swap Chain Image 변수 추가
		std::vector<VkImage> swapChainImages;
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