//MK: (코드 2-1) 필요한 Header 추가
#include <iostream>
#include <stdexcept>
#include <functional>
#include <cstdlib>
#include <util_init.hpp>

#define APP_SHORT_NAME "mkVulkanExample"

//MK: (코드 2-2) mkTriangle Class 생성
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
		}

		void mainLoop(){
		}

		void cleanup(){
		}
};

//MK: (코드 2-3) Android Main 함수
int sample_main(int argc, char *argv[]) {
	
	//MK: 코드 (2-4) Vulkan API를 로딩하는 부분	
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
