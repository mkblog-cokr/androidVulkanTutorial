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
			createSwapChain();
			createImageViews();
			createRenderPass();

			createGraphicsPipeline();

		}

		void mainLoop(){
		}

		void cleanup(){

			LOGI("MK: cleanup Function");

			//MK: (코드 1-4) Graphics Pipeline 제거하는 부분
			vkDestroyPipeline(device, graphicsPipeline, NULL);
			
			vkDestroyPipelineLayout(device, pipelineLayout, NULL);
			vkDestroyRenderPass(device, renderPass, nullptr);

			for(int i = 0; i < swapChainImageViews.size(); i++){
				vkDestroyImageView(device, swapChainImageViews[i], NULL);
			}

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

		void createSwapChain(){

			LOGI("MK: (Begin) createSwapChain Function");
			
			VkResult result = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &surfaceCapabilities);
			assert(result == VK_SUCCESS);
			LOGI("\tMK: Successfully Getting Surface Capability");
			LOGI("\t\tMK: minImageCount - %d, maxImageCount - %d", surfaceCapabilities.minImageCount, surfaceCapabilities.maxImageCount);

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

			selectedImageCount = surfaceCapabilities.minImageCount + 1;
			if(surfaceCapabilities.maxImageCount > 0 && selectedImageCount > surfaceCapabilities.maxImageCount){
				selectedImageCount = surfaceCapabilities.maxImageCount;
			}
			LOGI("\tMK: Selected Image Count - %d", selectedImageCount);

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

			uint32_t imageCount;
			result = vkGetSwapchainImagesKHR(device, swapChain, &imageCount, NULL);
			assert(result == VK_SUCCESS);
			swapChainImages.resize(imageCount);
			result = vkGetSwapchainImagesKHR(device, swapChain, &imageCount, swapChainImages.data());
			assert(result == VK_SUCCESS);
			LOGI("MK: Successfully Retrieveing %d Swap Chain Images (If not, should not see this print)", imageCount);

			LOGI("MK: (End) createSwapChain Function");

		}

		void createImageViews(){

			LOGI("MK: (Begin) createImageViews Function");

			swapChainImageViews.resize(swapChainImages.size());

			for(int i = 0; i < swapChainImageViews.size(); i++){
				VkImageViewCreateInfo createInfo = {};
				createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
				createInfo.pNext = NULL;
				createInfo.flags = 0;
				createInfo.image = swapChainImages[i];
				createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
				createInfo.format = selectedSurfaceFormat.format;
				createInfo.components.r = VK_COMPONENT_SWIZZLE_R;
				createInfo.components.g = VK_COMPONENT_SWIZZLE_G;
				createInfo.components.b = VK_COMPONENT_SWIZZLE_B;
				createInfo.components.a = VK_COMPONENT_SWIZZLE_A;
				createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT ;
				createInfo.subresourceRange.baseMipLevel = 0;
				createInfo.subresourceRange.levelCount = 1;
				createInfo.subresourceRange.baseArrayLayer = 0;
				createInfo.subresourceRange.layerCount = 1;

				VkResult result = vkCreateImageView(device, &createInfo, NULL, &swapChainImageViews[i]);
				assert(result == VK_SUCCESS);
				LOGI("MK:\t Successfully Create %d Image View (if not, should not see this print)", i);
			}
			
			LOGI("MK: (End) createImageViews Function");

		}

		void createGraphicsPipeline(){

			LOGI("MK: (Begin) createGraphicsPipeline Function");

			static const char *vertShaderText=
			"#version 450\n"
			"#extension GL_ARB_separate_shader_objects : enable\n"
			"vec2 positions[3] = vec2[](vec2(0.0, -0.5), vec2(0.5, 0.5), vec2(-0.5, 0.5));\n"
			"vec3 colors[3] = vec3[](vec3(1.0, 0.0, 0.0), vec3(0.0, 1.0, 0.0), vec3(0.0, 0.0, 1.0));\n"
			"layout(location = 0) out vec3 fragColor;\n"
			"void main() {\n"
			"gl_Position = vec4(positions[gl_VertexIndex], 0.0, 1.0);\n"
			"fragColor = colors[gl_VertexIndex];}\n";

			static const char *fragShaderText=
			"#version 450\n"
			"#extension GL_ARB_separate_shader_objects : enable\n"
			"layout(location = 0) in vec3 fragColor;\n"
			"layout(location = 0) out vec4 outColor;\n"
			"void main() {\n"
			"outColor = vec4(fragColor, 1.0);}\n";

			std::vector<unsigned int> vtxSpv;
			std::vector<unsigned int> fragSpv;

			init_glslang();

			bool retVal;
			retVal = GLSLtoSPV(VK_SHADER_STAGE_VERTEX_BIT, vertShaderText, vtxSpv);
			assert(retVal == true);
			LOGI("\tMK: Vertex Code is converted to SPV");

			retVal = GLSLtoSPV(VK_SHADER_STAGE_FRAGMENT_BIT, fragShaderText, fragSpv);
			assert(retVal == true);
			LOGI("\tMK: Fragment Code is converted to SPV");

			VkShaderModule vertShaderModule;
			VkShaderModule fragShaderModule;

			VkShaderModuleCreateInfo createInfo = {};
			createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
			createInfo.pNext = NULL;
			createInfo.flags = 0;
			createInfo.codeSize = vtxSpv.size() * sizeof(unsigned int);
			createInfo.pCode = vtxSpv.data();

			VkResult result = vkCreateShaderModule(device, &createInfo, NULL, &vertShaderModule);
			assert(result == VK_SUCCESS);
			LOGI("\tMK: Vertex Module is created\n");
			
			createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
			createInfo.pNext = NULL;
			createInfo.flags = 0;
			createInfo.codeSize = fragSpv.size() * sizeof(unsigned int);
			createInfo.pCode = fragSpv.data();

			result = vkCreateShaderModule(device, &createInfo, NULL, &fragShaderModule);
			assert(result == VK_SUCCESS);
			LOGI("\tMK: Fragment Module is created\n");

			shaderStages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
			shaderStages[0].pNext = NULL;
			shaderStages[0].flags = 0;
			shaderStages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
			shaderStages[0].module = vertShaderModule;
			shaderStages[0].pName = "main";
			shaderStages[0].pSpecializationInfo = NULL;

			shaderStages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
			shaderStages[1].pNext = NULL;
			shaderStages[1].flags = 0;
			shaderStages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
			shaderStages[1].module = fragShaderModule;
			shaderStages[1].pName = "main";
			shaderStages[1].pSpecializationInfo = NULL;

			VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
			vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
			vertexInputInfo.pNext = NULL;
			vertexInputInfo.flags = 0;
			vertexInputInfo.vertexBindingDescriptionCount = 0;
			vertexInputInfo.pVertexBindingDescriptions = NULL;
			vertexInputInfo.vertexAttributeDescriptionCount = 0;
			vertexInputInfo.pVertexAttributeDescriptions = NULL;

			VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
			inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
			inputAssembly.pNext = NULL;
			inputAssembly.flags = 0;
			inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST; 
			inputAssembly.primitiveRestartEnable = VK_FALSE;

			VkViewport viewport = {};
			viewport.x = 0.0f;
			viewport.y = 0.0f;
			viewport.width = (float) selectedExtent.width;
			viewport.height = (float) selectedExtent.height;
			viewport.minDepth = 0.0f;
			viewport.maxDepth = 1.0f;

			VkRect2D scissor;
			scissor.offset = {0, 0};
			scissor.extent = selectedExtent;

			VkPipelineViewportStateCreateInfo viewportState = {};
			viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
			viewportState.pNext = NULL;
			viewportState.flags = 0;
			viewportState.viewportCount = 1;
			viewportState.pViewports = &viewport;
			viewportState.scissorCount = 1;
			viewportState.pScissors = &scissor;

			VkPipelineRasterizationStateCreateInfo rasterizer = {};
			rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
			rasterizer.pNext = NULL;
			rasterizer.flags = 0;
			rasterizer.depthClampEnable = VK_FALSE;
			rasterizer.rasterizerDiscardEnable = VK_FALSE;
			rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
			rasterizer.lineWidth = 1.0f;
			rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
			rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
			rasterizer.depthBiasEnable = VK_FALSE;
			rasterizer.depthBiasConstantFactor = 0.0f;
			rasterizer.depthBiasClamp = 0.0f;
			rasterizer.depthBiasSlopeFactor = 0.0f;

			VkPipelineMultisampleStateCreateInfo multisampling = {};
			multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
			multisampling.pNext = NULL;
			multisampling.flags = 0;
			multisampling.pSampleMask = NULL;
			multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
			multisampling.sampleShadingEnable = VK_FALSE;
			multisampling.alphaToCoverageEnable = VK_FALSE;
			multisampling.alphaToOneEnable = VK_FALSE;
			multisampling.minSampleShading = 0.0f;

			VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
			colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
			colorBlendAttachment.blendEnable = VK_FALSE;
			//MK: 나머지 값은 선택임. 아무 값을 저장하지 않아도 됨
			colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE; 
			colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
			colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD; 
			colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
			colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO; 
			colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

			VkPipelineColorBlendStateCreateInfo colorBlending = {};
			colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
			colorBlending.logicOpEnable = VK_FALSE;
			colorBlending.attachmentCount = 1;
			colorBlending.pAttachments = &colorBlendAttachment;
			//MK: 나머지 값은 선택임. 아무 값을 저장하지 않아도 됨
			colorBlending.logicOp = VK_LOGIC_OP_COPY;			
			colorBlending.blendConstants[0] = 0.0f;
			colorBlending.blendConstants[1] = 0.0f;
			colorBlending.blendConstants[2] = 0.0f;
			colorBlending.blendConstants[3] = 0.0f;

			VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
			pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
			pipelineLayoutInfo.pNext = NULL;
			pipelineLayoutInfo.setLayoutCount = 0;
			pipelineLayoutInfo.pSetLayouts = NULL;
			pipelineLayoutInfo.pushConstantRangeCount = 0;
			pipelineLayoutInfo.pPushConstantRanges = NULL;

			result = vkCreatePipelineLayout(device, &pipelineLayoutInfo, NULL, &pipelineLayout);

			assert(result == VK_SUCCESS);
			LOGI("MK: Successfully Create Pipeline Layout (If not, should not see this print)");

			//MK: (코드 1-2) VkGraphicsPipelineCreateInfo Struct를 생성하는 코드
			VkGraphicsPipelineCreateInfo pipelineInfo = {};
			pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
			pipelineInfo.pNext = NULL;
			pipelineInfo.flags = 0;
			pipelineInfo.stageCount = 2;
			pipelineInfo.pStages = shaderStages;
			pipelineInfo.pVertexInputState = &vertexInputInfo;
			pipelineInfo.pInputAssemblyState = &inputAssembly;
			pipelineInfo.pTessellationState = NULL;
			pipelineInfo.pViewportState = &viewportState;
			pipelineInfo.pRasterizationState = &rasterizer;
			pipelineInfo.pMultisampleState = &multisampling;
			pipelineInfo.pDepthStencilState = NULL;
			pipelineInfo.pColorBlendState = &colorBlending;
			pipelineInfo.pDynamicState = NULL;
			pipelineInfo.layout = pipelineLayout;
			pipelineInfo.renderPass = renderPass;
			pipelineInfo.subpass = 0;
			pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
			pipelineInfo.basePipelineIndex =-1;

			//MK: (코드 1-3) Graphics Pipeline 생성
			result = vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, NULL, &graphicsPipeline);

			assert(result == VK_SUCCESS);
			LOGI("MK: Successfully Create Graphics Pipeline (If not, should not see this print)");

			vkDestroyShaderModule(device, fragShaderModule, NULL);
			vkDestroyShaderModule(device, vertShaderModule, NULL);

			finalize_glslang();

			LOGI("MK: (End) createGraphicsPipeline Function");
			
		}

		void createRenderPass(){

			LOGI("MK: createRenderPass Function");

			VkAttachmentDescription colorAttachment = {};
			colorAttachment.flags = 0;
			colorAttachment.format = selectedSurfaceFormat.format;
			colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
			colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
			colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
			colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

			VkAttachmentReference colorAttachmentRef = {};
			colorAttachmentRef.attachment = 0;
			colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

			VkSubpassDescription subpass = {};
			subpass.flags = 0;
			subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
			subpass.inputAttachmentCount = 0;
			subpass.pInputAttachments = NULL;
			subpass.colorAttachmentCount = 1;
			subpass.pColorAttachments = &colorAttachmentRef;
			subpass.pResolveAttachments = NULL;
			subpass.pDepthStencilAttachment = NULL;
			subpass.preserveAttachmentCount = 0;
			subpass.pPreserveAttachments = NULL;

			VkRenderPassCreateInfo renderPassInfo = {};
			renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
			renderPassInfo.pNext = NULL;
			renderPassInfo.flags = 0;
			renderPassInfo.attachmentCount = 1;
			renderPassInfo.pAttachments = &colorAttachment;
			renderPassInfo.subpassCount = 1;
			renderPassInfo.pSubpasses = &subpass;
			renderPassInfo.dependencyCount = 0;
			renderPassInfo.pDependencies = NULL;

			VkResult result = vkCreateRenderPass(device, &renderPassInfo, NULL, &renderPass);
			assert(result == VK_SUCCESS);
			LOGI("MK: Successfully Create Render Pass (If not, should not see this print)");

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

		VkSurfaceCapabilitiesKHR surfaceCapabilities;
		std::vector<VkSurfaceFormatKHR> surfaceFormats;
		std::vector<VkPresentModeKHR> presentModes;

		VkSurfaceFormatKHR selectedSurfaceFormat;
		VkPresentModeKHR selectedPresentMode;
		VkExtent2D selectedExtent;
		uint32_t selectedImageCount;

		VkSwapchainKHR swapChain;

		std::vector<VkImage> swapChainImages;

		std::vector<VkImageView> swapChainImageViews;

		VkPipelineShaderStageCreateInfo shaderStages[2];

		VkPipelineLayout pipelineLayout;

		//MK: (코드 1-1) Graphics Pipeline 저장할 Object를 생성
		VkPipeline graphicsPipeline;

		VkRenderPass renderPass;
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