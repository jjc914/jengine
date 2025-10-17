#include "editor_gui.hpp"

#include "engine/core/graphics/device.hpp"

#include "panels/scene_view_panel.hpp"
#include "panels/inspector_panel.hpp"
#include "panels/console_panel.hpp"

#include "engine/core/debug/assert.hpp"

#include <imgui_internal.h>
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

namespace editor::gui {

EditorGui::EditorGui(const engine::core::graphics::Instance& instance,
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

    _is_first_frame = true;

    _panels.emplace_back(std::make_unique<panels::SceneViewPanel>());
    _panels.emplace_back(std::make_unique<panels::InspectorPanel>());
    _panels.emplace_back(std::make_unique<panels::ConsolePanel>());
}

EditorGui::~EditorGui() {
    vkDeviceWaitIdle(_device);
    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    
    ImGui::DestroyContext();
}

void EditorGui::on_gui(GuiContext& context) {
    ENGINE_ASSERT(context.command_buffer, "EditorGui expects a valid command buffer.");
    void* cb = context.command_buffer;

    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();    
    ImGui::NewFrame();

    // host flags
    ImGuiWindowFlags host_window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
    const ImGuiViewport* viewport = ImGui::GetMainViewport();

    ImGui::SetNextWindowPos(viewport->Pos);
    ImGui::SetNextWindowSize(viewport->Size);
    ImGui::SetNextWindowViewport(viewport->ID);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);

    host_window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse |
                         ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
                         ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

    // background clear
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::Begin("EditorDockspace", nullptr, host_window_flags);
    ImGui::PopStyleVar();
    ImGui::PopStyleVar(2);

    ImGuiID dockspace_id = ImGui::GetID("MainDockspace");
    ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_PassthruCentralNode;
    ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);

    // menu bar
    if (ImGui::BeginMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            ImGui::MenuItem("New Scene");
            ImGui::MenuItem("Open Scene");
            ImGui::MenuItem("Save Scene");
            ImGui::Separator();
            if (ImGui::MenuItem("Exit")) { /* ... */ }
            ImGui::EndMenu();
        }
        ImGui::EndMenuBar();
    }

    ImGui::End(); // end of host dockspace

    // set up dock layout
    if (_is_first_frame) {
        _is_first_frame = false;

        ImGui::DockBuilderRemoveNode(dockspace_id);
        ImGui::DockBuilderAddNode(dockspace_id, ImGuiDockNodeFlags_DockSpace);
        ImGui::DockBuilderSetNodeSize(dockspace_id, viewport->Size);

        ImGuiID dock_main_id = dockspace_id;
        ImGuiID dock_right_id  = ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Right, 0.25f, nullptr, &dock_main_id);
        ImGuiID dock_bottom_id = ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Down,  0.25f, nullptr, &dock_main_id);

        ImGui::DockBuilderDockWindow("Scene View", dock_main_id);
        ImGui::DockBuilderDockWindow("Inspector", dock_right_id);
        ImGui::DockBuilderDockWindow("Console", dock_bottom_id);

        ImGui::DockBuilderFinish(dockspace_id);
    }

    // draw panels
    for (auto& panel : _panels)
        panel->draw(context);

    // render imgui
    ImGui::Render();
    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), static_cast<VkCommandBuffer>(cb));

    if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
        ImGui::UpdatePlatformWindows();
        ImGui::RenderPlatformWindowsDefault();
    }
}

ImTextureID EditorGui::register_texture(VkImageView image_view) {
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

void EditorGui::unregister_texture(ImTextureID texture_id) {
    ImGui_ImplVulkan_RemoveTexture(reinterpret_cast<VkDescriptorSet>(texture_id));
    _registered_textures.erase(
        std::remove(_registered_textures.begin(), _registered_textures.end(), texture_id),
        _registered_textures.end()
    );
}

} // namespace editor::gui
