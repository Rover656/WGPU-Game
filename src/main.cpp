#include <iostream>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#define WEBGPU_CPP_IMPLEMENTATION
#include <webgpu/webgpu.hpp>
#include <glfw3webgpu.h>

int main() {
    wgpu::InstanceDescriptor instanceDescriptor = wgpu::Default;

    wgpu::Instance instance = wgpu::createInstance(instanceDescriptor);
    if (!instance) {
        std::cerr << "Failed to create WebGPU instance" << std::endl;
        return 1;
    }

    std::cout << "Got instance " << instance << std::endl;

    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW." << std::endl;
        return 1;
    }

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    GLFWwindow* window = glfwCreateWindow(640, 480, "Learn WebGPU", nullptr, nullptr);
    if (!window) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        return 1;
    }

    std::cout << "Requesting adapter..." << std::endl;

    wgpu::Surface surface = glfwGetWGPUSurface(instance, window);

    wgpu::RequestAdapterOptions adapterOpts = wgpu::Default;
    adapterOpts.compatibleSurface = surface;

    wgpu::Adapter adapter = instance.requestAdapter(adapterOpts);
    if (!adapter) {
        std::cerr << "Failed to request adapter" << std::endl;
        return 1;
    }

    std::cout << "Got adapter " << adapter << std::endl;

    std::cout << "Requesting device..." << std::endl;

    wgpu::DeviceDescriptor deviceDescriptor = wgpu::Default;
    deviceDescriptor.label = "WGPU Device";
    deviceDescriptor.requiredFeaturesCount = 0;
    deviceDescriptor.requiredFeatures = nullptr;
    deviceDescriptor.defaultQueue.label = "Default Queue";

    wgpu::Device device = adapter.requestDevice(deviceDescriptor);
    if (!device) {
        std::cerr << "Failed to request device" << std::endl;
        return 1;
    }

    wgpu::Queue queue = device.getQueue();

    auto onDeviceError = [](wgpu::ErrorType type, char const* message) {
        std::cout << "Uncaptured device error: type " << type;
        if (message) std::cout << " (" << message << ")";
        std::cout << std::endl;
    };
    device.setUncapturedErrorCallback(onDeviceError);

    wgpu::SwapChainDescriptor swapChainDescriptor = wgpu::Default;
    swapChainDescriptor.width = 640;
    swapChainDescriptor.height = 480;

    wgpu::TextureFormat swapChainFormat = surface.getPreferredFormat(adapter);
    swapChainDescriptor.format = swapChainFormat;
    swapChainDescriptor.usage = wgpu::TextureUsage::RenderAttachment;
    swapChainDescriptor.presentMode = wgpu::PresentMode::Fifo;

    wgpu::SwapChain swapChain = device.createSwapChain(surface, swapChainDescriptor);
    std::cout << "Swapchain: " << swapChain << std::endl;

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        wgpu::TextureView nextTexture = swapChain.getCurrentTextureView();
        //std::cout << "nextTexture: " << nextTexture << std::endl;

        if (!nextTexture) {
            std::cerr << "Cannot acquire next swap chain texture" << std::endl;
            break;
        }

        wgpu::CommandEncoderDescriptor commandEncoderDesc = wgpu::Default;
        commandEncoderDesc.label = "Command Encoder";

        wgpu::CommandEncoder encoder = device.createCommandEncoder(commandEncoderDesc);

        wgpu::RenderPassDescriptor renderPassDesc = wgpu::Default;

        wgpu::RenderPassColorAttachment renderPassColorAttachment = wgpu::Default;
        renderPassColorAttachment.view = nextTexture;
        renderPassColorAttachment.resolveTarget = nullptr;
        renderPassColorAttachment.loadOp = wgpu::LoadOp::Clear;
        renderPassColorAttachment.storeOp = wgpu::StoreOp::Store;
        renderPassColorAttachment.clearValue = wgpu::Color { 0.9, 0.1, 0.2, 1.0 };

        renderPassDesc.colorAttachmentCount = 1;
        renderPassDesc.colorAttachments = &renderPassColorAttachment;

        renderPassDesc.depthStencilAttachment = nullptr;
        renderPassDesc.timestampWriteCount = 0;
        renderPassDesc.timestampWrites = nullptr;

        wgpu::RenderPassEncoder renderPass = encoder.beginRenderPass(renderPassDesc);
        renderPass.end();

        nextTexture.release();

        wgpu::CommandBufferDescriptor cmdBufferDescriptor = wgpu::Default;
        cmdBufferDescriptor.label = "Command buffer";
        wgpu::CommandBuffer command = encoder.finish(cmdBufferDescriptor);
        queue.submit(1, &command);

        swapChain.present();
    }

    swapChain.release();
    device.release();
    adapter.release();
    surface.release();
    glfwDestroyWindow(window);
    glfwTerminate();
    instance.release();
    return 0;
}
