
/* -*- Mode: C++; tabrr-width: 4; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 *   Copyright 2023-Present Couchbase, Inc.
 *
 *   Licensed under the Apache License, Version 2.0 (the "License");
 *   you may not use this file except in compliance with the License.
 *   You may obtain a copy of the License at
 *
 *       http://www.apache.org/licenses/LICENSE-2.0
 *
 *   Unless required by applicable law or agreed to in writing, software
 *   distributed under the License is distributed on an "AS IS" BASIS,
 *   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *   See the License for the specific language governing permissions and
 *   limitations under the License.
 */

#include "config-gui.hxx"

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include <GLFW/glfw3.h>

#include <random>
#include <vector>

struct Box {
  int state; // 0=red, 1=yellow, 2=green
  double timer;
  double delay;
};

namespace cbc
{
void
config_graphical_ui(const std::function<std::string()>& /* get_config */)
{
  // Initialize GLFW
  if (glfwInit() == 0) {
    return; // FIXME: error
  }

  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

  GLFWwindow* window = glfwCreateWindow(1280, 720, "Box Dashboard", nullptr, nullptr);
  if (window == nullptr) {
    glfwTerminate();
    return; // FIXME: error
  }
  glfwMakeContextCurrent(window);
  glfwSwapInterval(1);

  // Setup ImGui
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGui_ImplGlfw_InitForOpenGL(window, true);
  ImGui_ImplOpenGL3_Init("#version 330");

  // Initialize 1024 boxes with random delays
  std::vector<Box> boxes(1024);
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_real_distribution<double> dist(0.5, 2.0);

  for (auto& box : boxes) {
    box.state = 0;
    box.timer = 0.0;
    box.delay = dist(gen);
  }

  double last_time = glfwGetTime();

  // Main loop
  while (glfwWindowShouldClose(window) == 0) {
    glfwPollEvents();

    double current_time = glfwGetTime();
    double delta_time = current_time - last_time;
    last_time = current_time;

    // Update box states
    for (auto& box : boxes) {
      box.timer += delta_time;
      if (box.timer >= box.delay) {
        box.timer = 0.0;
        box.state = (box.state + 1) % 3;
        box.delay = dist(gen);
      }
    }

    // Start ImGui frame
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    // Create dashboard window
    ImGui::Begin("Box Dashboard", nullptr, ImGuiWindowFlags_NoResize);

    // Draw 32x32 grid of boxes
    for (std::size_t i = 0; i < 32; i++) {
      for (std::size_t j = 0; j < 32; j++) {
        std::size_t idx = (i * 32) + j;
        ImVec4 color;
        switch (boxes[idx].state) {
          case 0:
            color = ImVec4(1.0, 0.0, 0.0, 1.0);
            break;
          case 1:
            color = ImVec4(1.0, 1.0, 0.0, 1.0);
            break;
          case 2:
            color = ImVec4(0.0, 1.0, 0.0, 1.0);
            break;
          default:
            color = ImVec4(0.0, 0.0, 0.0, 0.0);
        }

        ImGui::PushStyleColor(ImGuiCol_Button, color);
        ImGui::PushID(static_cast<int>(idx));
        ImGui::Button("", ImVec2(20, 20));
        ImGui::PopID();
        ImGui::PopStyleColor();

        if (j < 31) {
          ImGui::SameLine();
        }
      }
    }

    ImGui::End();

    // Rendering
    ImGui::Render();
    int display_w = 0;
    int display_h = 0;
    glfwGetFramebufferSize(window, &display_w, &display_h);
    glViewport(0, 0, display_w, display_h);
    glClearColor(0.1F, 0.1F, 0.1F, 1.0F);
    glClear(GL_COLOR_BUFFER_BIT);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    glfwSwapBuffers(window);
  }

  // Cleanup
  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImGui::DestroyContext();
  glfwDestroyWindow(window);
  glfwTerminate();
}
} // namespace cbc
