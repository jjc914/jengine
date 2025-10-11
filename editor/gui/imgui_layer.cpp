#include "imgui_layer.hpp"

#include "engine/core/graphics/device.hpp"

#include "engine/core/debug/assert.hpp"

#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_vulkan.h>

#include <wk/wulkan.hpp>

#include <memory>

namespace {

static void CheckVkResult(VkResult err) {
    if (err != VK_SUCCESS) {
#ifndef NDEBUG
        ENGINE_ASSERT(false, "Vulkan call failed (VkResult = %d)", static_cast<int>(err));
#else
        engine::core::debug::Logger::get_singleton().fatal(
            "Vulkan call failed with VkResult = {}", static_cast<int>(err));
#endif
    }
}

}

namespace editor::ui {

ImGuiLayer::ImGuiLayer(const engine::core::graphics::Instance& instance,
    const engine::core::graphics::Device& device,
    const engine::core::graphics::Pipeline& pipeline,
    const engine::core::window::Window& window) : _device(static_cast<VkDevice>(device.native_device()))
{
    IMGUI_CHECKVERSION();

    ImGui::CreateContext();

    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

    ImGui::StyleColorsDark();

    ImGui_ImplGlfw_InitForVulkan(static_cast<GLFWwindow*>(window.native_window()), true);

    ImGui_ImplVulkan_InitInfo init_info{};
    init_info.ApiVersion = VK_API_VERSION_1_3;
    init_info.Instance = static_cast<VkInstance>(instance.native_instance());
    init_info.PhysicalDevice = static_cast<VkPhysicalDevice>(device.native_physical_device());
    init_info.Device = _device;
    init_info.QueueFamily = device.native_graphics_queue_family();
    init_info.Queue = static_cast<VkQueue>(device.native_graphics_queue());
    init_info.DescriptorPool = static_cast<VkDescriptorPool>(device.native_descriptor_pool());
    init_info.MinImageCount = 2;
    init_info.ImageCount = 3;
    init_info.PipelineInfoMain.RenderPass = static_cast<VkRenderPass>(pipeline.native_render_pass());
    init_info.PipelineInfoMain.Subpass = 0;
    init_info.PipelineInfoMain.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
    init_info.CheckVkResultFn = CheckVkResult;

    ImGui_ImplVulkan_Init(&init_info);

    _sampler = wk::Sampler(
        _device,
        wk::SamplerCreateInfo{}
            .set_mag_filter(VK_FILTER_LINEAR)
            .set_min_filter(VK_FILTER_LINEAR)
            .set_mipmap_mode(VK_SAMPLER_MIPMAP_MODE_LINEAR)
            .set_address_mode_u(VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE)
            .set_address_mode_v(VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE)
            .set_address_mode_w(VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE)
            .set_min_lod(-1000.0f)
            .set_max_lod(1000.0f)
            .set_max_anisotropy(1.0f)
            .set_border_color(VK_BORDER_COLOR_INT_OPAQUE_BLACK)
            .to_vk()
    );

    // _descriptor_set = ImGui_ImplVulkan_AddTexture(_sampler.handle(), )
}

ImGuiLayer::~ImGuiLayer() {
    vkDeviceWaitIdle(_device);
    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    
    ImGui::DestroyContext();
}

void ImGuiLayer::begin_frame() {
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    
    ImGui::NewFrame();
}

void ImGuiLayer::end_frame(void* cb) {
    ImGui::Render();
    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), static_cast<VkCommandBuffer>(cb));

    ImGui::UpdatePlatformWindows();
    ImGui::RenderPlatformWindowsDefault();
}

ImTextureID ImGuiLayer::register_texture(VkImageView image_view) {
    ImTextureID id = reinterpret_cast<ImTextureID>(
        ImGui_ImplVulkan_AddTexture(
            _sampler.handle(),
            image_view,
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
        )
    );

    _registered_textures.push_back(id);
    return id;
}

void ImGuiLayer::unregister_texture(ImTextureID texture_id) {
    ImGui_ImplVulkan_RemoveTexture(reinterpret_cast<VkDescriptorSet>(texture_id));
    _registered_textures.erase(
        std::remove(_registered_textures.begin(), _registered_textures.end(), texture_id),
        _registered_textures.end()
    );
}

} // namespace editor::ui
