#include <iostream>
#include <stdexcept>
#include <functional>
#include <cstdlib>
#include <util_init.hpp>

#define APP_SHORT_NAME "mkVulkanExample"

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
		}

		void mainLoop(){
		}

		void cleanup(){
			//MK: (코드 1-6) Instance Destory 코드
			vkDestroyInstance(instance, nullptr);
		}

		void createInstance(){

			//MK: (코드 1-1) Application 정보를 입력함 (Optional)
			LOGI("MK: Create VkApplicationInfo");
			VkApplicationInfo appInfo = {};
			appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
			appInfo.pNext = NULL;
			appInfo.pApplicationName = APP_SHORT_NAME;
			appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
			appInfo.pEngineName = "No Engine";
			appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
			appInfo.apiVersion = VK_API_VERSION_1_0;

			//MK: (코드 1-3) 지원하는 Extension을 모두 Enable 함
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

			//MK: (코드 1- 4) 지원하는 Layer을 모두 Enable 함
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

			//MK: (코드 1-2) Instance 정보 입력
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

			//MK: (코드 1-5) Instance 생성
			VkResult result = vkCreateInstance(&createInfo, nullptr, &instance);
			
			assert(result == VK_SUCCESS);
			LOGI("MK: Successfully Create Instance (If not, should be able to see this print)");

		}

		VkInstance instance;
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
