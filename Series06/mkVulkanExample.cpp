#include <iostream>
#include <stdexcept>
#include <functional>
#include <cstdlib>
#include <util_init.hpp>

#define APP_SHORT_NAME "mkVulkanExample"

//MK: (코드 1-4) vk 함수 Function Pointer (FP)를 찾는 코드
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
			//MK: Surface 생성 함수 호출
			createSurface();
		}

		void mainLoop(){
		}

		void cleanup(){
			//MK: (코드 1-5) Surface Destroy 코드 
			vkDestroySurfaceKHR(instance, surface, NULL);
			vkDestroyInstance(instance, nullptr);
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
			vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
			std::vector<VkExtensionProperties> extensions(extensionCount);
			vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions.data());
			LOGI("MK: Instance Extensions Count - %d", extensionCount);
			std::vector<const char *> instanceExtensions;
            for (const auto& extension : extensions){
                LOGI("MK: Instance Extension - %s", extension.extensionName);
				//MK: 필요 Extnesion은 String으로 보내야 함
				instanceExtensions.push_back(extension.extensionName);
            }

			uint32_t layerCount = 0;
			vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
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

			VkResult result = vkCreateInstance(&createInfo, nullptr, &instance);
			
			assert(result == VK_SUCCESS);
			LOGI("MK: Successfully Create Instance (If not, should not see this print)");

		}

		//MK: (코드 1-2) createSurface 함수 추가
		void createSurface(){
			
			//MK: VkAndroidSurfaceCreateInfoKHR Struct 작성
			VkAndroidSurfaceCreateInfoKHR createInfo = {};
			createInfo.sType = VK_STRUCTURE_TYPE_ANDROID_SURFACE_CREATE_INFO_KHR;
			createInfo.pNext = nullptr;
			createInfo.flags = 0;
			createInfo.window = AndroidGetApplicationWindow();
			
			//MK: vkCreateAndroidSurfaceKHR 함수 포인터 검색
			MK_GET_INSTANCE_PROC_ADDR(instance, CreateAndroidSurfaceKHR);
			
			//MK: (코드 1-3) Surface 생성
			VkResult result = fpCreateAndroidSurfaceKHR(instance, &createInfo, nullptr, &surface);

			assert(result == VK_SUCCESS);
			LOGI("MK: Successfully Create Surface (If not, should not see this print)");
		}

		VkInstance instance;

		//MK: (코드 1-1) VkSurfaceKHR 변수 추가
		VkSurfaceKHR surface;
   		PFN_vkCreateAndroidSurfaceKHR fpCreateAndroidSurfaceKHR;
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
