#include <iostream>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#define WEBGPU_CPP_IMPLEMENTATION
#include <webgpu/webgpu.hpp>
#include <glfw3webgpu.h>

const char* shaderSource = R"(
@vertex
fn vs_main(@builtin(vertex_index) in_vertex_index: u32) -> @builtin(position) vec4f {
    var p = vec2f(0.0, 0.0);
    if (in_vertex_index == 0u) {
        p = vec2f(-0.5, -0.5);
    } else if (in_vertex_index == 1u) {
        p = vec2f(0.5, -0.5);
    } else {
        p = vec2f(0.0, 0.5);
    }
    return vec4f(p, 0.0, 1.0);
}

@fragment
fn fs_main() -> @location(0) vec4f {
    return vec4f(0.0, 0.4, 1.0, 1.0);
}
)";

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

    wgpu::ShaderModuleDescriptor shaderModuleDescriptor = wgpu::Default;
#ifdef WEBGPU_BACKEND_WGPU
    shaderModuleDescriptor.hintCount = 0;
    shaderModuleDescriptor.hints = nullptr;
#endif

    wgpu::ShaderModuleWGSLDescriptor shaderCodeDesc;
    shaderCodeDesc.chain.next = nullptr;
    shaderCodeDesc.chain.sType = wgpu::SType::ShaderModuleWGSLDescriptor;
    shaderModuleDescriptor.nextInChain = &shaderCodeDesc.chain;

    shaderCodeDesc.code = shaderSource;

    wgpu::ShaderModule shaderModule = device.createShaderModule(shaderModuleDescriptor);

    wgpu::RenderPipelineDescriptor pipelineDescriptor = wgpu::Default;

    pipelineDescriptor.vertex.bufferCount = 0;
    pipelineDescriptor.vertex.buffers = nullptr;

    pipelineDescriptor.vertex.module = shaderModule;
    pipelineDescriptor.vertex.entryPoint = "vs_main";
    pipelineDescriptor.vertex.constantCount = 0;
    pipelineDescriptor.vertex.constants = nullptr;

    pipelineDescriptor.primitive.topology = wgpu::PrimitiveTopology::TriangleList;
    pipelineDescriptor.primitive.stripIndexFormat = wgpu::IndexFormat::Undefined;
    pipelineDescriptor.primitive.frontFace = wgpu::FrontFace::CCW;
    pipelineDescriptor.primitive.cullMode = wgpu::CullMode::None;

    wgpu::FragmentState fragmentState = wgpu::Default;
    fragmentState.module = shaderModule;
    fragmentState.entryPoint = "fs_main";
    fragmentState.constantCount = 0;
    fragmentState.constants = nullptr;

    wgpu::BlendState blendState = wgpu::Default;
    blendState.color.srcFactor = wgpu::BlendFactor::SrcAlpha;
    blendState.color.dstFactor = wgpu::BlendFactor::OneMinusSrcAlpha;
    blendState.color.operation = wgpu::BlendOperation::Add;

    blendState.alpha.srcFactor = wgpu::BlendFactor::Zero;
    blendState.alpha.dstFactor = wgpu::BlendFactor::One;
    blendState.alpha.operation = wgpu::BlendOperation::Add;

    wgpu::ColorTargetState colorTargetState = wgpu::Default;
    colorTargetState.format = swapChainFormat;
    colorTargetState.blend = &blendState;
    colorTargetState.writeMask = wgpu::ColorWriteMask::All;

    fragmentState.targetCount = 1;
    fragmentState.targets = &colorTargetState;

    pipelineDescriptor.fragment = &fragmentState;
    pipelineDescriptor.depthStencil = nullptr;

    pipelineDescriptor.multisample.count = 1;
    pipelineDescriptor.multisample.mask = ~0u;
    pipelineDescriptor.multisample.alphaToCoverageEnabled = false;

    pipelineDescriptor.layout = nullptr;

    wgpu::RenderPipeline pipeline = device.createRenderPipeline(pipelineDescriptor);

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

        renderPass.setPipeline(pipeline);
        renderPass.draw(3, 1, 0, 0);

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
