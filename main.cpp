// Include WebGPU header
#include "webgpu-utils.h"

#include <webgpu/webgpu.h>

#include <iostream>
#include <cassert>
#include <vector>
#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif // __EMSCRIPTEN__

#include <GLFW/glfw3.h>
#include <glfw3webgpu.h>


// GLFW Class
class Application
{
public:
	bool Initialize();

	void Terminate();

	// Main Loop
	void MainLoop();

	bool IsRunning();

private:
	GLFWwindow *window;
	WGPUDevice device;
	WGPUQueue queue;
	WGPUSurface surface;
};

bool Application::Initialize()
{
	// Init Windows
	glfwInit();
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API); // No OpenGL context
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE); // No resizing
	window = glfwCreateWindow(800, 600, "WebGPU Example", nullptr, nullptr);

	// Init Instance
	WGPUInstance instance = wgpuCreateInstance(nullptr);

	// Get Adapter
	std::cout << "Requesting adapter..." << std::endl;
	surface = glfwGetWGPUSurface(instance, window);

	WGPURequestAdapterOptions adapterOpts = {};
	adapterOpts.nextInChain = nullptr;
	adapterOpts.compatibleSurface = surface;
	WGPUAdapter adapter = requestAdapterSync(instance, &adapterOpts);
	std::cout << "Got adapter: " << adapter << std::endl;

	// Release Instance
	wgpuInstanceRelease(instance);

	// Get Device
	std::cout << "Requesting device..." << std::endl;
	WGPUDeviceDescriptor deviceDesc = {};
	deviceDesc.nextInChain = nullptr;
	deviceDesc.label = "MyDevice";
	deviceDesc.requiredFeatureCount = 0; // No specific features required
	deviceDesc.requiredLimits = nullptr; // No specific limits required
	deviceDesc.defaultQueue.nextInChain = nullptr;
	deviceDesc.defaultQueue.label = "Default Queue";

	// Callback func for device lost
	// deviceDesc.deviceLostCallback = [](WGPUDeviceLostReason reason, char const* message, void* /* pUserData */) { // // Deprecated in WGPU
	// 	std::cout << "Device lost: reason " << reason;
	// 	if (message) std::cout << " (" << message << ")";
	// 	std::cout << std::endl;
	// };

	WGPUDeviceLostCallbackInfo deviceLostCbInfo = {};
	deviceLostCbInfo.nextInChain = nullptr;
	deviceLostCbInfo.mode = WGPUCallbackMode_WaitAnyOnly;
	deviceLostCbInfo.callback = [](WGPUDevice const* device, WGPUDeviceLostReason reason,char const * message, void * /* pUserData */) {
		std::cout << "Device lost callback: " << (device ? *device : nullptr);
		std::cout << " reason: " << reason;
		if (message) std::cout << " (" << message << ")";
		std::cout << std::endl;
	};
	deviceLostCbInfo.userdata = nullptr; // No user data

	deviceDesc.deviceLostCallbackInfo = deviceLostCbInfo;


	device = requestDeviceSync(adapter, &deviceDesc);
	std::cout << "Got device: " << device << std::endl;

	// Callback func for device error
	auto onDeviceError = [](WGPUErrorType type, char const * message, void * /* pUserData */) {
		std::cout << "Uncaptured device error: type " << type;
		if (message) std::cout << " (" << message << ")";
		std::cout << std::endl;
	};
	wgpuDeviceSetUncapturedErrorCallback(device, onDeviceError, nullptr /* pUserData */);

	// Release Adapter
	wgpuAdapterRelease(adapter);

	queue = wgpuDeviceGetQueue(device);

	return true;
}

void Application::Terminate()
{
	wgpuQueueRelease(queue);
	wgpuSurfaceRelease(surface);
	wgpuDeviceRelease(device);
	glfwDestroyWindow(window);
	glfwTerminate();
}

void Application::MainLoop()
{
	// glfw
	glfwPollEvents();
#if defined(WEBGPU_BACKEND_DAWN)
	wgpuDeviceTick(device);
#elif defined(WEBGPU_BACKEND_WGPU)
	wgpuDevicePoll(device, false, nullptr);
#endif
}

bool Application::IsRunning()
{
	return !glfwWindowShouldClose(window) && glfwGetKey(window, GLFW_KEY_ESCAPE) != GLFW_PRESS;
}



int main (int, char**) {
	// init app
	Application app;
	if (!app.Initialize()) {
		std::cerr << "Failed to initialize application." << std::endl;
		return -1;
	}
	// Main loop
#ifdef __EMSCRIPTEN__
	// Equivalent of the main loop when using Emscripten:
	auto callback = [](void *arg) {
		//                   ^^^ 2. We get the address of the app in the callback.
		Application* pApp = reinterpret_cast<Application*>(arg);
		//                  ^^^^^^^^^^^^^^^^ 3. We force this address to be interpreted
		//                                      as a pointer to an Application object.
		pApp->MainLoop(); // 4. We can use the application object
	};
	emscripten_set_main_loop_arg(callback, &app, 0, true);
	//                                     ^^^^ 1. We pass the address of our application object.
#else // __EMSCRIPTEN__
	while (app.IsRunning()) {
		app.MainLoop();
	}
#endif // __EMSCRIPTEN__

	app.Terminate();


	return 0;
}
