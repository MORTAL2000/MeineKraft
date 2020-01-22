#include "meinekraft.hpp"

#include <chrono>

#include "imgui/imgui.h"
#include "rendering/primitives.hpp"
#include "rendering/renderer.hpp"
#include "../include/imgui/imgui_impl_sdl.h"
#include "rendering/camera.hpp"
#include "nodes/model.hpp"
#include "util/filesystem.hpp"
#include "scene/world.hpp"
#include "rendering/debug_opengl.hpp"
#include "nodes/skybox.hpp"
#include "scene/world.hpp"
#include "rendering/graphicsbatch.hpp"
#include "util/config.hpp"

// TODO: Update try to ImGui some day

// ImGui global GUI settings
struct {
  bool render_system_window = true;  // Render system information and config. window
  bool scene_graph_window = false;   // Scene graph of Graphic entities
  bool logger_window = false;        // TODO: Logger of engine messages
  bool console_window = false;       // TODO: Console for engine commands
  bool help_window = false;          // TODO: Helpful keyboard shortcuts, displayed on first launch
  bool about_window = false;         // TODO: Displays some information about the application
} Gui;

// Helper to display a little (?) mark which shows a tooltip when hovered.
// In your own code you may want to display an actual icon if you are using a merged icon fonts (see docs/FONTS.txt)
static void ImGui_HelpMarker(const std::string& str) {
  ImGui::TextDisabled("(?)");
  if (ImGui::IsItemHovered()) {
    ImGui::BeginTooltip();
    ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
    ImGui::TextUnformatted(str.c_str());
    ImGui::PopTextWrapPos();
    ImGui::EndTooltip();
  }
}

/// Used for the Render system frame time graph
static ImVec4 imgui_color_for_value(const float v, const float a, const float b) {
  ImVec4 color;
 
  if (v <= a) {
    color = ImVec4(0.35f, 1.0f, 0.0f, 0.57f); // Good
  } else if (v <= b) {
    color = ImVec4(1.0f, 1.0f, 0.0f, 0.57f); // OK
  } else {
    color = ImVec4(1.0, 0.35f, 0.0f, 0.57f); // Bad
  }

  return color;
}

/// Sets the custom engine styling for ImGui
void imgui_styling() {
  ImGuiStyle* style = &ImGui::GetStyle();

  style->WindowPadding = ImVec2(15, 15);
  style->WindowRounding = 5.0f;
  style->WindowTitleAlign = ImVec2(0.5, 0.5);
  style->FramePadding = ImVec2(5, 5);
  style->FrameRounding = 4.0f;
  style->ItemSpacing = ImVec2(12, 8);
  style->ItemInnerSpacing = ImVec2(10, 6);
  style->IndentSpacing = 25.0f;
  style->ScrollbarSize = 15.0f;
  style->ScrollbarRounding = 9.0f;
  style->GrabMinSize = 20.0f;
  style->GrabRounding = 3.0f;

  style->Colors[ImGuiCol_Text] = ImVec4(0.80f, 0.80f, 0.83f, 1.00f);
  style->Colors[ImGuiCol_TextDisabled] = ImVec4(0.24f, 0.23f, 0.29f, 1.00f);
  style->Colors[ImGuiCol_WindowBg] = ImVec4(0.06f, 0.05f, 0.07f, 0.80f);
  style->Colors[ImGuiCol_ChildWindowBg] = ImVec4(0.07f, 0.07f, 0.09f, 1.00f);
  style->Colors[ImGuiCol_PopupBg] = ImVec4(0.07f, 0.07f, 0.09f, 1.00f);
  style->Colors[ImGuiCol_Border] = ImVec4(0.80f, 0.80f, 0.83f, 0.88f);
  style->Colors[ImGuiCol_BorderShadow] = ImVec4(0.92f, 0.91f, 0.88f, 0.00f);
  style->Colors[ImGuiCol_FrameBg] = ImVec4(0.20f, 0.19f, 0.22f, 1.00f);
  style->Colors[ImGuiCol_FrameBgHovered] = ImVec4(0.20f, 0.19f, 0.25f, 1.00f);
  style->Colors[ImGuiCol_FrameBgActive] = ImVec4(0.56f, 0.56f, 0.58f, 1.00f);
  style->Colors[ImGuiCol_TitleBg] = ImVec4(0.10f, 0.09f, 0.12f, 1.00f);
  style->Colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.40f, 0.40f, 0.40f, 0.75f);
  style->Colors[ImGuiCol_TitleBgActive] = ImVec4(0.07f, 0.07f, 0.09f, 1.00f);
  style->Colors[ImGuiCol_MenuBarBg] = ImVec4(0.10f, 0.09f, 0.12f, 1.00f);
  style->Colors[ImGuiCol_ScrollbarBg] = ImVec4(0.10f, 0.09f, 0.12f, 1.00f);
  style->Colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.80f, 0.80f, 0.83f, 0.31f);
  style->Colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.56f, 0.56f, 0.58f, 1.00f);
  style->Colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.06f, 0.05f, 0.07f, 1.00f);
  style->Colors[ImGuiCol_ComboBg] = ImVec4(0.19f, 0.18f, 0.21f, 1.00f);
  style->Colors[ImGuiCol_CheckMark] = ImVec4(0.40f, 1.0f, 0.40f, 0.31f);
  style->Colors[ImGuiCol_SliderGrab] = ImVec4(0.80f, 0.80f, 0.83f, 0.31f);
  style->Colors[ImGuiCol_SliderGrabActive] = ImVec4(0.06f, 0.05f, 0.07f, 1.00f);
  style->Colors[ImGuiCol_Button] = ImVec4(0.10f, 0.09f, 0.12f, 1.00f);
  style->Colors[ImGuiCol_ButtonHovered] = ImVec4(0.35f, 0.35f, 0.35f, 1.00f);
  style->Colors[ImGuiCol_ButtonActive] = ImVec4(0.56f, 0.56f, 0.58f, 1.00f);
  style->Colors[ImGuiCol_Header] = ImVec4(0.10f, 0.09f, 0.12f, 1.00f);
  style->Colors[ImGuiCol_HeaderHovered] = ImVec4(0.35f, 0.35f, 0.35f, 1.00f);
  style->Colors[ImGuiCol_HeaderActive] = ImVec4(0.06f, 0.05f, 0.07f, 1.00f);
  style->Colors[ImGuiCol_Column] = ImVec4(0.56f, 0.56f, 0.58f, 1.00f);
  style->Colors[ImGuiCol_ColumnHovered] = ImVec4(0.24f, 0.23f, 0.29f, 1.00f);
  style->Colors[ImGuiCol_ColumnActive] = ImVec4(0.56f, 0.56f, 0.58f, 1.00f);
  style->Colors[ImGuiCol_ResizeGrip] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
  style->Colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.56f, 0.56f, 0.58f, 1.00f);
  style->Colors[ImGuiCol_ResizeGripActive] = ImVec4(0.06f, 0.05f, 0.07f, 1.00f);
  style->Colors[ImGuiCol_CloseButton] = ImVec4(0.40f, 0.39f, 0.38f, 0.16f);
  style->Colors[ImGuiCol_CloseButtonHovered] = ImVec4(0.40f, 0.39f, 0.38f, 0.39f);
  style->Colors[ImGuiCol_CloseButtonActive] = ImVec4(0.40f, 0.39f, 0.38f, 1.00f);
  style->Colors[ImGuiCol_PlotLines] = ImVec4(0.40f, 0.39f, 0.88f, 0.63f);
  style->Colors[ImGuiCol_PlotLinesHovered] = ImVec4(1.0f, 1.00f, 1.0f, 1.00f);
  style->Colors[ImGuiCol_PlotHistogram] = ImVec4(0.40f, 0.39f, 0.38f, 0.63f);
  style->Colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.0f, 1.00f, 1.0f, 1.00f);
  style->Colors[ImGuiCol_TextSelectedBg] = ImVec4(0.55f, 0.55f, 0.55f, 0.43f);
  style->Colors[ImGuiCol_ModalWindowDarkening] = ImVec4(1.00f, 0.98f, 0.95f, 0.73f);
}

/// Main engine constructor
MeineKraft::MeineKraft() {
  SDL_Init(SDL_INIT_EVERYTHING);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, OPENGL_MINOR_VERSION);
  SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
  SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_DEBUG_FLAG);
  auto window_flags = SDL_WINDOW_OPENGL | SDL_WINDOW_MOUSE_CAPTURE;
  window = SDL_CreateWindow("MeineKraft", 0, 0, HD.width, HD.height, window_flags);
  SDL_GLContext context = SDL_GL_CreateContext(window);
  if (!context) { Log::error(std::string(SDL_GetError())); }
  SDL_GL_SetSwapInterval(0); // Disables vsync

  glewExperimental = (GLboolean) true;
  glewInit();

  // Create MeineKraft folder structure
  Filesystem::create_directory(Filesystem::tmp);

  OpenGLContextInfo gl_context_info(4, OPENGL_MINOR_VERSION);

  atexit(IMG_Quit);
  IMG_Init(IMG_INIT_JPG | IMG_INIT_PNG); 
  
  ImGui_ImplSdlGL3_Init(window);

  renderer = new Renderer(HD);
  renderer->update_projection_matrix(70.0f, HD);

  imgui_styling();
}

void MeineKraft::init() {
  bool success = false;
  const auto config = Config::load_config(success);
  if (success) {
    const std::string path = config["scene"]["path"].get<std::string>();
    const std::string name = config["scene"]["name"].get<std::string>();
    // renderer->scene = new Scene("/home/alexander/Desktop/Meinekraft/BoomBox/", "BoomBox.gltf");
    renderer->scene = new Scene(Filesystem::home + path, name);
    // renderer->scene = new Scene("/home/alexander/Desktop/Meinekraft/MetalRoughSpheres/", "MetalRoughSpheres.gltf");
    // renderer->scene->load_models_from("/home/alexander/Desktop/Meinekraft/Suzanne/", "Suzanne.gltf");
    // renderer->scene->load_models_from("/home/alexander/Desktop/Meinekraft/MetalRoughSpheres/", "MetalRoughSpheres.gltf");
    // renderer->scene->load_models_from("/home/alexander/Desktop/Meinekraft/BoomBox/", "BoomBox.gltf");
  } else {
    // TODO: Load default scene or smt
    Log::error("Failed to load config.json.");
  }

  renderer->init();
}

MeineKraft::~MeineKraft() {
  if (renderer) { delete renderer; }
  ImGui_ImplSdlGL3_Shutdown();
  SDL_DestroyWindow(window);
  SDL_Quit();
}

void MeineKraft::mainloop() {
  World world;

  bool toggle_mouse_capture = true;
  bool done = false;
  auto last_tick = std::chrono::high_resolution_clock::now();
  auto current_tick = last_tick;
  int64_t delta = 0;

  /// Delta values
  const int num_deltas = 150;
  float deltas[num_deltas];
  bool take_screenshot = false;

  while (!done) {
      current_tick = std::chrono::high_resolution_clock::now();
      delta = std::chrono::duration_cast<std::chrono::milliseconds>(current_tick - last_tick).count();
      last_tick = current_tick;

      /// Window handling
      static bool throttle_rendering = false;

      /// Process input
      SDL_Event event{};
      while (SDL_PollEvent(&event) != 0) {
        ImGui_ImplSdlGL3_ProcessEvent(&event);
        switch (event.type) {
        case SDL_MOUSEMOTION:
          if (toggle_mouse_capture) { break; }
          renderer->scene->camera->pitch += event.motion.yrel;
          renderer->scene->camera->yaw += event.motion.xrel;
          renderer->scene->camera->direction = renderer->scene->camera->recalculate_direction();
          break;

        case SDL_KEYDOWN:
          switch (event.key.keysym.sym) {
          case SDLK_w:
            renderer->scene->camera->move_forward(true);
            break;
          case SDLK_a:
            renderer->scene->camera->move_left(true);
            break;
          case SDLK_s:
            renderer->scene->camera->move_backward(true);
            break;
          case SDLK_d:
            renderer->scene->camera->move_right(true);
            break;
          case SDLK_q:
            renderer->scene->camera->move_down(true);
            break;
          case SDLK_e:
            renderer->scene->camera->move_up(true);
            break;
          case SDLK_TAB:
            toggle_mouse_capture = !toggle_mouse_capture;
            break;
          case SDLK_ESCAPE:
            done = true;
            break;
          }
          break;

        case SDL_KEYUP:
          switch (event.key.keysym.sym) {
          case SDLK_w:
            renderer->scene->camera->move_forward(false);
            break;
          case SDLK_a:
            renderer->scene->camera->move_left(false);
            break;
          case SDLK_s:
            renderer->scene->camera->move_backward(false);
            break;
          case SDLK_d:
            renderer->scene->camera->move_right(false);
            break;
          case SDLK_q:
            renderer->scene->camera->move_down(false);
            break;
          case SDLK_e:
            renderer->scene->camera->move_up(false);
          }

        case SDL_WINDOWEVENT:
          switch (event.window.event) {
            case SDL_WINDOWEVENT_RESIZED:
              Resolution screen;
              SDL_GetWindowSize(window, &screen.width, &screen.height);
              renderer->update_projection_matrix(70.0f, screen);
              break;
            case SDL_WINDOWEVENT_FOCUS_GAINED:
              throttle_rendering = false;
              break;
            case SDL_WINDOWEVENT_FOCUS_LOST:
              throttle_rendering = true; // FIXME: May cause problems when RenderDoc is attached
              break;
          }
          break;
        case SDL_QUIT:
          done = true;
          break;
      }
    }
    renderer->scene->camera->position = renderer->scene->camera->update(delta);

    /// Run all actions
    ActionSystem::instance().execute_actions(renderer->state.frame, delta);

    /// Let the game do its thing
    world.tick();

    /// Render the world
    if (!throttle_rendering) {      
      renderer->render(delta);
    }

    if (take_screenshot) {
      const Vec3f *pixels = renderer->take_screenshot();
      Filesystem::save_image_as_ppm(Filesystem::tmp + "screenshot", pixels, renderer->screen.width, renderer->screen.height);
      delete pixels;
      take_screenshot = false;
    }

    /// ImGui - Debug instruments
    if (!throttle_rendering) {
      // pass_started("ImGui"); // FIXME: Render pass started fence for debug info

      ImGui_ImplSdlGL3_NewFrame(window);
      auto io = ImGui::GetIO();

      //  Main window
      {
        // ImGui::ShowTestWindow(); // NOTE: Shows demo of ImGui widgets

        if (ImGui::BeginMainMenuBar()) {
          if (ImGui::BeginMenu("MeineKraft")) {
            if (ImGui::MenuItem("Quit", "ESC")) { done = true; }
            if (ImGui::MenuItem("Screenshot")) { take_screenshot = true; }
            if (ImGui::MenuItem("Hide GUI")) {  } // TODO: Implement
            ImGui::EndMenu();
          }

          if (ImGui::BeginMenu("Windows")) {
            if (ImGui::MenuItem("Render system", "CTRL+R")) { Gui.render_system_window = !Gui.render_system_window; }
            if (ImGui::MenuItem("Logger system", "CTRL+L")) { Gui.logger_window = !Gui.logger_window; }
            if (ImGui::MenuItem("Scene graph",   "CTRL+S")) { Gui.scene_graph_window = !Gui.scene_graph_window; }
            ImGui::Separator();
            if (ImGui::MenuItem("Close all ..")) {  }; // TODO: Implement ...
            if (ImGui::MenuItem("Reset position ..")) {  }; // TODO: Implement ...
            ImGui::EndMenu();
          }

          ImGui::Text("(%.1f FPS)", io.Framerate);

          ImGui::EndMainMenuBar();
        }

        // Render system
        if (Gui.render_system_window) {
          ImGui::Begin("Render system", &Gui.render_system_window);

          ImGui::PushItemWidth(90.0f);

          static size_t i = -1; i = (i + 1) % num_deltas;
          deltas[i] = float(delta);
          const ImVec4 color = imgui_color_for_value(float(delta), 16.0f, 20.0f);
          ImGui::PushStyleColor(ImGuiCol_PlotLines, color);
          ImGui::PlotLines("", deltas, num_deltas, 0, "ms / frame", 0.0f, 50.0f, ImVec2(ImGui::GetWindowWidth(), 80));
          ImGui::PopStyleColor();

          ImGui::Text("Average %lu ms/frame (%.1f FPS)", delta, io.Framerate);
          ImGui::Text("Frame: %lu", renderer->state.frame);
          ImGui::Text("Resolution: (%u, %u)", renderer->screen.width, renderer->screen.height);
          ImGui::Text("Draw calls per frame: %u", renderer->state.draw_calls);
          ImGui::Text("Render passes: %u", renderer->state.render_passes);
          ImGui::Text("Render passes total time (ms): %.1f", renderer->state.total_execution_time / 1'000'000.0f);
          // TODO: Change resolution, memory usage, textures, render pass execution times, etc

          if (ImGui::CollapsingHeader("Global settings")) {
            ImGui::Checkbox("Normal mapping", &renderer->state.lighting.normalmapping);
            ImGui::SliderInt("Downsample modifier", &renderer->state.lighting.downsample_modifier, 1, 4);
            ImGui::SameLine(); ImGui_HelpMarker("GI is performed in lower resolution by factor 1/x");

            if (ImGui::TreeNode("GPU view frustum culling")) {
              ImGui::Text("Execution time (ms): %.2f / %.2f%% total", renderer->state.culling.execution_time / 1'000'000.0f, 100.0f * float(renderer->state.culling.execution_time) / float(renderer->state.total_execution_time));
              ImGui::Checkbox("Enabled##culling", &renderer->state.culling.enabled);
              ImGui::TreePop();
            }

            if (ImGui::TreeNode("Global buffer generation")) {
              ImGui::Text("Execution time (ms): %.2f / %.2f%% total", renderer->state.gbuffer.execution_time / 1'000'000.0f, 100.0f * float(renderer->state.gbuffer.execution_time) / float(renderer->state.total_execution_time));
              ImGui::TreePop();
            }

            if (ImGui::TreeNode("Final blit pass")) {
              ImGui::Text("Execution time (ms): %.2f / %.2f%% total", renderer->state.blit.execution_time / 1'000'000.0f, 100.0f * float(renderer->state.blit.execution_time) / float(renderer->state.total_execution_time));
              ImGui::TreePop();
            }
          }

          if (ImGui::CollapsingHeader("Direct shadows")) {
            ImGui::Text("Execution time (ms): %.2f / %.2f%% total", renderer->state.shadow.execution_time / 1'000'000.0f, 100.0f * float(renderer->state.shadow.execution_time) / float(renderer->state.total_execution_time));
            ImGui::Checkbox("Enabled", &renderer->state.lighting.direct);
            ImGui::Text("Shadowmap resolution: (%u, %u)", renderer->state.shadow.SHADOWMAP_W, renderer->state.shadow.SHADOWMAP_H);
            ImGui::SliderInt("Resolution modifier", &renderer->state.shadow.shadowmap_resolution_step, 1, 5);
            ImGui::SameLine(); ImGui_HelpMarker("Changes the shadowmap texture size");

            static int s = 0; // Selection
            ImGui::Combo("Algorithm", &s, "Plain \0 PCF \0 VCT \0");
            ImGui::SameLine(); ImGui_HelpMarker("Shadow algorithm applied by the directional light \n Plain: Classic shadowmapping \n PCF: Percentage-closer filtering \n VCT: Voxel cone traced shadows");
            renderer->state.shadow.algorithm = ShadowAlgorithm(s);

            switch (renderer->state.shadow.algorithm) {
            case ShadowAlgorithm::Plain:
                ImGui::InputFloat("Shadow bias", &renderer->state.shadow.bias);
                ImGui::SameLine(); ImGui_HelpMarker("Offset along normal to avoid shadow acne");
                break;
            case ShadowAlgorithm::PercentageCloserFiltering:
                ImGui::InputFloat("Shadow bias", &renderer->state.shadow.bias);
                ImGui::SameLine(); ImGui_HelpMarker("Offset along normal to avoid shadow acne");
                ImGui::InputInt("Samples", &renderer->state.shadow.pcf_samples);
                break;
            case ShadowAlgorithm::VCT:
                ImGui::InputFloat("Shadow cone aperature (deg.)", &renderer->state.shadow.vct_cone_aperature);
                break;
            }
          }

          if (ImGui::CollapsingHeader("Voxelization")) {
            ImGui::Text("Execution time (ms): %.2f / %.2f%% total", renderer->state.voxelization.execution_time / 1'000'000.0f, 100.0f * float(renderer->state.voxelization.execution_time) / float(renderer->state.total_execution_time));

            if (ImGui::Button("Voxelize")) {
              renderer->state.voxelization.voxelize = true;
            }
            ImGui::SameLine();
            ImGui::Checkbox("Always voxelize", &renderer->state.voxelization.always_voxelize);

            ImGui::Checkbox("Conservative voxelization", &renderer->state.voxelization.conservative_rasterization);
          }

          if (ImGui::CollapsingHeader("Voxel cone tracing", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::Text("Execution time (ms): %.2f / %.2f%% total", renderer->state.vct.execution_time / 1'000'000.0f, 100.0f * float(renderer->state.vct.execution_time) / float(renderer->state.total_execution_time));

            ImGui::InputFloat("Ambient decay factor", &renderer->state.vct.ambient_decay);

            ImGui::Checkbox("Indirect", &renderer->state.lighting.indirect);
            ImGui::SameLine();
            ImGui::Checkbox("Specular", &renderer->state.lighting.specular);
            ImGui::SameLine();
            ImGui::Checkbox("Ambient", &renderer->state.lighting.ambient);
            ImGui::SameLine();
            ImGui_HelpMarker("Note ambient is only available when 'Indirect' is used.");

            ImGui::SliderInt("# diffuse cones", &renderer->state.vct.num_diffuse_cones, 4, renderer->state.vct.MAX_DIFFUSE_CONES);

            ImGui::InputFloat("Roughness aperature (deg.)", &renderer->state.vct.roughness_aperature);
            ImGui::InputFloat("Metallic aperature (deg.)", &renderer->state.vct.metallic_aperature);
          }

          if (ImGui::CollapsingHeader("Bilateral filtering", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::Text("Execution time (ms): %.2f / %.2f%% total", renderer->state.bilateral_filtering.execution_time / 1'000'000.0f, 100.0f * float(renderer->state.bilateral_filtering.execution_time) / float(renderer->state.total_execution_time));

            ImGui::Checkbox("Enabled##bilateral", &renderer->state.bilateral_filtering.enabled);

            ImGui::Text("Filter:");
            ImGui::Checkbox("Direct##filtering", &renderer->state.bilateral_filtering.direct);
            ImGui::SameLine();
            ImGui::Checkbox("Indirect##filtering", &renderer->state.bilateral_filtering.indirect);

            ImGui::Checkbox("Specular##filtering", &renderer->state.bilateral_filtering.specular);
            ImGui::SameLine();
            ImGui::Checkbox("Ambient##filtering", &renderer->state.bilateral_filtering.ambient);

            ImGui::Text("Weights:");
            ImGui::Checkbox("Position", &renderer->state.bilateral_filtering.position_weight);
            ImGui::SameLine(); ImGui::SliderFloat("Sigma##Position", &renderer->state.bilateral_filtering.position_sigma, 0.05f, 5.0f);

            ImGui::Separator();

            ImGui::Checkbox("Normal", &renderer->state.bilateral_filtering.normal_weight);
            ImGui::SameLine(); ImGui::SliderFloat("Sigma##Normal", &renderer->state.bilateral_filtering.normal_sigma, 0.05f, 5.0f);
          }

          if (ImGui::CollapsingHeader("Bilinear upsampling")) {
            ImGui::Text("Execution time (ms): %.2f / %.2f%% total", renderer->state.bilinear_upsample.execution_time / 1'000'000.0f, 100.0f * float(renderer->state.bilinear_upsample.execution_time) / float(renderer->state.total_execution_time));

            ImGui::Checkbox("Enabled##bilinear", &renderer->state.bilinear_upsample.enabled);

            ImGui::Checkbox("Indirect##bilinear", &renderer->state.bilinear_upsample.indirect);
            ImGui::SameLine();
            ImGui::Checkbox("Specular##bilinear", &renderer->state.bilinear_upsample.specular);
            ImGui::SameLine();
            ImGui::Checkbox("Ambient##bilinear", &renderer->state.bilinear_upsample.ambient);
          }

          ImGui::PopItemWidth();
          ImGui::End();
        }

        if (Gui.scene_graph_window) {
          ImGui::Begin("Scene graph", &Gui.scene_graph_window, ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_AlwaysAutoResize);

          if (ImGui::BeginMenuBar()) {
            if (ImGui::BeginMenu("New##scene-graph")) {
              if (ImGui::MenuItem("Model")) {
                // TODO: Model loader GUI
              }
              if (ImGui::BeginMenu("Geometric primitive")) {
                if (ImGui::MenuItem("Cube")) {
                  world.spawn_entity(MeshPrimitive::Cube);
                }
                if (ImGui::MenuItem("Sphere")) {
                  world.spawn_entity(MeshPrimitive::Sphere);
                }
                ImGui::EndMenu();
              }
              ImGui::EndMenu();
              ImGui::Separator();
              if (ImGui::BeginMenu("Light")) {
                if (ImGui::MenuItem("Pointlight")) {
                  // TODO: Spawn pointlight
                }
                ImGui::EndMenu();
              }
            }
            ImGui::EndMenuBar();
          }

          if (ImGui::CollapsingHeader("Camera", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::InputFloat3("Position##camera", &renderer->scene->camera->position.x);
            ImGui::InputFloat3("Direction##camera", &renderer->scene->camera->direction.x);
            // ImGui::InputFloat("FoV", &renderer->scene->camera->FoV); // TODO: Camera adjustable FoV
            ImGui::SameLine();
            if (ImGui::Button("Reset")) { renderer->scene->reset_camera(); }
          }

          // Directional light
          if (ImGui::CollapsingHeader("Directional light")) {
            ImGui::InputFloat3("Direction##directional_light", &renderer->directional_light.direction.x);
            ImGui::SameLine(); ImGui_HelpMarker("Direction is negative.");

            ImGui::ColorEdit3("Intensity##directional_light", &renderer->directional_light.intensity.x);
            ImGui::SameLine(); ImGui_HelpMarker("Intensity or color of the light."); // FIXME: May or may not exceed 1.0??
          }

          // Point lights
          const std::string pointlights_title = "Point lights (" + std::to_string(renderer->pointlights.size()) + ")";
          if (ImGui::CollapsingHeader(pointlights_title.c_str())) {
            for (size_t i = 0; i < renderer->pointlights.size(); i++) {
              ImGui::PushID(&renderer->pointlights[i]);
              const std::string str = std::to_string(i);
              if (ImGui::CollapsingHeader(str.c_str())) {
                ImGui::InputFloat3("Position##pointlight", &renderer->pointlights[i].position.x);
                ImGui::ColorEdit3("Intensity##pointlight", &renderer->pointlights[i].intensity.x);
                ImGui::SameLine(); ImGui_HelpMarker("Intensity or color of the light."); // FIXME: May or may not exceed 1.0??
              }
              ImGui::PopID();
            }
          }

          // Graphics batches
          const std::string graphicsbatches_title = "Graphics batches (" + std::to_string(renderer->graphics_batches.size()) + ")";
          if (ImGui::CollapsingHeader(graphicsbatches_title.c_str())) {
            for (size_t batch_num = 0; batch_num < renderer->graphics_batches.size(); batch_num++) {
              const auto& batch = renderer->graphics_batches[batch_num];
             
              const std::string batch_title = "Batch #" + std::to_string(batch_num + 1) + " (" + std::to_string(batch.entity_ids.size()) + ")";
              if (ImGui::CollapsingHeader(batch_title.c_str())) {

                const std::string member_title = "Members##" + batch_title;
                if (ImGui::CollapsingHeader(member_title.c_str())) {
                  for (const auto& id : batch.entity_ids) {
                    ImGui::Text("Entity id: %lu", id);

                    const std::string* name = NameSystem::instance().get_name_from_entity_referenced(id);
                    ImGui::Text("Name: %s", name->c_str());
                    // ImGui::InputText("Name", name->c_str(), name->size()); // TODO: Change Entity name

                    TransformComponent* transform = TransformSystem::instance().lookup_referenced(id);
                    ImGui::PushID(transform);
                    ImGui::InputFloat3("Position", &transform->position.x);
                    ImGui::InputFloat3("Rotation", &transform->rotation.x);
                    ImGui::InputFloat("Scale", &transform->scale);
                    ImGui::PopID();

                    if (ImGui::Button("Remove")) {
                      renderer->remove_component(id);
                    }
                    ImGui::SameLine();
                    if (ImGui::Button("Clone")) {
                      // TODO: Clone Entity: entity-clone();
                    }
                  }
                }
              }
            }
          }

          ImGui::End();
        }

        if (Gui.logger_window) {
          ImGui::Begin("Logger system", &Gui.logger_window);
          // TODO: Implement logger system window
          // for (const auto& item : Logger::msgs) {
          // }
          ImGui::End();
        }

        ImGui::Render();
        // pass_ended(); // FIXME: Render pass handling outside Renderer not allowed
      }
    }
    SDL_GL_SwapWindow(window);
  }
  Config::save_scene(renderer->scene);
}
