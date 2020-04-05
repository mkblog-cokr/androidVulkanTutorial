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
			//MK: Physical Device를 선택하는 함수 호출
			pickPhysicalDevice();
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

		//MK: (코드 1-2) Physical Device를 선택하는 함수
		void pickPhysicalDevice(){
			LOGI("MK: pickPhysicalDevice Function");

			uint32_t deviceCount = 0;
			vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
			assert(deviceCount != 0);

			LOGI("MK: Physical Device Count - %d", deviceCount);
	
			//MK: 안드로이드 단말의 경우 보통(?) 1개의 GPU만 가지고 있음
			//MK: (코드 1-3) Physical Device 정보를 가져오는 코드
			std::vector<VkPhysicalDevice> devices(deviceCount);
			vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

			//MK: (코드 1-4) Query 한 단말 정보를 출력하기 위한 코드
			for(const auto &device : devices){
				printPhysicalDeviceInfo(device);
			}

			//MK: 단말에 GPU가 1개 밖에 없다는 가정으로 제일 처음 GPU를 선택함
			physicalDevice = devices[0];

			assert(physicalDevice != VK_NULL_HANDLE);
			LOGI("MK: Successfully Select Physical Device (If not, should not see this print)");
		}

		//MK: (코드 1-5) GPU 정보를 간단하게 출력하는 함수
		void printPhysicalDeviceInfo(VkPhysicalDevice device){
			VkPhysicalDeviceProperties deviceProperties;
			VkPhysicalDeviceFeatures deviceFeatures;
			vkGetPhysicalDeviceProperties(device, &deviceProperties);
			vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

			LOGI("MK: Physical Device Name - %s", deviceProperties.deviceName);
			LOGI("MK: Physical Device - geometryShader (%d)", deviceFeatures.geometryShader);
			LOGI("MK: Physical Device - shaderInt64 (%d)", deviceFeatures.shaderInt64);
		}

		VkInstance instance;

		VkSurfaceKHR surface;
   		PFN_vkCreateAndroidSurfaceKHR fpCreateAndroidSurfaceKHR;

   		//MK: (코드 1-1) Physical Device 변수 추가
   		VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
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
