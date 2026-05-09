#include "client/app/guiApp.h"

#include <array>

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <GLFW/glfw3.h>
#include <GL/gl.h>

#include "client/net/httpClient.h"
#include "client/app/screens.h"
#include "client/ui/appState.h"
#include "protocol/api.h"

namespace simpleelo::client::app {

namespace {

void applyMinimalistTheme() {
  ImGui::StyleColorsLight();
  ImGuiStyle& style = ImGui::GetStyle();

  style.WindowRounding = 8.0f;
  style.ChildRounding = 8.0f;
  style.FrameRounding = 6.0f;
  style.PopupRounding = 8.0f;
  style.ScrollbarRounding = 8.0f;
  style.GrabRounding = 6.0f;
  style.TabRounding = 6.0f;
  style.WindowBorderSize = 1.0f;
  style.ChildBorderSize = 1.0f;
  style.FrameBorderSize = 1.0f;
  style.PopupBorderSize = 1.0f;
  style.ItemSpacing = ImVec2(10.0f, 9.0f);
  style.FramePadding = ImVec2(10.0f, 8.0f);
  style.WindowPadding = ImVec2(14.0f, 14.0f);

  ImVec4* colors = style.Colors;
  colors[ImGuiCol_Text] = ImVec4(0.12f, 0.12f, 0.12f, 1.00f);
  colors[ImGuiCol_TextDisabled] = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
  colors[ImGuiCol_WindowBg] = ImVec4(0.97f, 0.97f, 0.96f, 1.00f);
  colors[ImGuiCol_ChildBg] = ImVec4(0.99f, 0.99f, 0.99f, 1.00f);
  colors[ImGuiCol_PopupBg] = ImVec4(0.99f, 0.99f, 0.99f, 0.98f);
  colors[ImGuiCol_Border] = ImVec4(0.86f, 0.86f, 0.84f, 1.00f);
  colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
  colors[ImGuiCol_FrameBg] = ImVec4(0.95f, 0.95f, 0.94f, 1.00f);
  colors[ImGuiCol_FrameBgHovered] = ImVec4(0.92f, 0.92f, 0.91f, 1.00f);
  colors[ImGuiCol_FrameBgActive] = ImVec4(0.89f, 0.89f, 0.88f, 1.00f);
  colors[ImGuiCol_TitleBg] = ImVec4(0.95f, 0.95f, 0.94f, 1.00f);
  colors[ImGuiCol_TitleBgActive] = ImVec4(0.93f, 0.93f, 0.92f, 1.00f);
  colors[ImGuiCol_MenuBarBg] = ImVec4(0.95f, 0.95f, 0.94f, 1.00f);
  colors[ImGuiCol_ScrollbarBg] = ImVec4(0.95f, 0.95f, 0.94f, 1.00f);
  colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.76f, 0.76f, 0.73f, 1.00f);
  colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.69f, 0.69f, 0.66f, 1.00f);
  colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.62f, 0.62f, 0.60f, 1.00f);
  colors[ImGuiCol_CheckMark] = ImVec4(0.22f, 0.36f, 0.52f, 1.00f);
  colors[ImGuiCol_SliderGrab] = ImVec4(0.34f, 0.49f, 0.67f, 1.00f);
  colors[ImGuiCol_SliderGrabActive] = ImVec4(0.27f, 0.42f, 0.60f, 1.00f);
  colors[ImGuiCol_Button] = ImVec4(0.90f, 0.92f, 0.94f, 1.00f);
  colors[ImGuiCol_ButtonHovered] = ImVec4(0.84f, 0.88f, 0.92f, 1.00f);
  colors[ImGuiCol_ButtonActive] = ImVec4(0.77f, 0.83f, 0.89f, 1.00f);
  colors[ImGuiCol_Header] = ImVec4(0.89f, 0.92f, 0.95f, 1.00f);
  colors[ImGuiCol_HeaderHovered] = ImVec4(0.83f, 0.88f, 0.93f, 1.00f);
  colors[ImGuiCol_HeaderActive] = ImVec4(0.77f, 0.84f, 0.90f, 1.00f);
  colors[ImGuiCol_Separator] = ImVec4(0.84f, 0.84f, 0.82f, 1.00f);
  colors[ImGuiCol_ResizeGrip] = ImVec4(0.69f, 0.73f, 0.78f, 0.60f);
  colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.53f, 0.61f, 0.70f, 0.80f);
  colors[ImGuiCol_ResizeGripActive] = ImVec4(0.42f, 0.52f, 0.62f, 1.00f);
  colors[ImGuiCol_Tab] = ImVec4(0.92f, 0.92f, 0.91f, 1.00f);
  colors[ImGuiCol_TabHovered] = ImVec4(0.84f, 0.88f, 0.92f, 1.00f);
  colors[ImGuiCol_TabActive] = ImVec4(0.80f, 0.86f, 0.92f, 1.00f);
  colors[ImGuiCol_TabUnfocused] = ImVec4(0.93f, 0.93f, 0.92f, 1.00f);
  colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.89f, 0.90f, 0.91f, 1.00f);
}

void setupUiFont(ImGuiIO& io) {
  // ImGui default font has no CJK glyphs, so Chinese text renders as tofu.
  const std::array<const char*, 6> fontCandidates = {
      "C:/Windows/Fonts/msyh.ttc",       // Microsoft YaHei
      "C:/Windows/Fonts/msyh.ttf",
      "C:/Windows/Fonts/simhei.ttf",     // SimHei
      "C:/Windows/Fonts/simsun.ttc",     // SimSun
      "C:/Windows/Fonts/NotoSansCJK-Regular.ttc",
      "C:/Windows/Fonts/arialuni.ttf"};

  for (const char* fontPath : fontCandidates) {
    if (io.Fonts->AddFontFromFileTTF(fontPath, 18.0f, nullptr, io.Fonts->GetGlyphRangesChineseFull()) != nullptr) {
      return;
    }
  }

  io.Fonts->AddFontDefault();
}

}  // namespace

int runGuiApp() {
  int width = 1080;
  int height = 720;

  if (!glfwInit()) {
    return 1;
  }

  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
  GLFWwindow* window = glfwCreateWindow(width, height, "SimpleElo Interactive Client", nullptr, nullptr);
  if (window == nullptr) {
    glfwTerminate();
    return 1;
  }
  glfwMakeContextCurrent(window);
  glfwSwapInterval(1);

  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO& io = ImGui::GetIO();
  (void)io;
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
  setupUiFont(io);
  applyMinimalistTheme();

  ImGui_ImplGlfw_InitForOpenGL(window, true);
  ImGui_ImplOpenGL3_Init("#version 130");

  simpleelo::client::ui::AppState state;
  while (!glfwWindowShouldClose(window)) {
    glfwPollEvents();

    int displayWidth = 0;
    int displayHeight = 0;
    glfwGetFramebufferSize(window, &displayWidth, &displayHeight);
    setViewportSize(displayWidth, displayHeight);

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    renderFrame(state);

    ImGui::Render();
    glViewport(0, 0, displayWidth, displayHeight);
    glClearColor(0.95f, 0.95f, 0.94f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    glfwSwapBuffers(window);
  }

  if (state.loggedIn && !std::string(state.selectedRoomId.data()).empty()) {
    try {
      const auto req = protocol::api::buildLeaveRoomRequest(state.token, state.selectedRoomId.data());
      (void)simpleelo::client::net::sendJsonLine(state.host.data(), state.port, req.dump());
    } catch (...) {
      // Ignore cleanup failure on process exit.
    }
  }

  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImGui::DestroyContext();

  glfwDestroyWindow(window);
  glfwTerminate();
  return 0;
}

}  // namespace simpleelo::client::app
