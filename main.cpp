#include <glad/glad.h>
#include "includes/GLFW/glfw3.h"
#include "includes/imgui/imgui.h"
#include "includes/imgui/backends/imgui_impl_glfw.h"
#include "includes/imgui/backends/imgui_impl_opengl3.h"

#include "Common.h"
#include "Physics.h"
#include "Render.h"

#include <iostream>
#include <vector>

#include <cmath>
#include <algorithm>

int numParticles = 1000;
bool isPaused = true;
std::vector<Vec2> predictedPositions;


void ResetSimulation(int& currentNum, std::vector<Particle>& particles, std::vector<Vec2>& renderPos, std::vector<float>& renderSpeeds) {
    particles.assign(currentNum, Particle());
    predictedPositions.resize(currentNum);
    renderPos.resize(currentNum);
    renderSpeeds.resize(currentNum);

    ResizePhysicsData(currentNum);

    for (int i = 0; i < currentNum; i++) {
        particles[i].x = (i % 20 - 10) * 0.1f;
        particles[i].y = (i / 20) * 0.1f;
        particles[i].vx = particles[i].vy = 0;
        particles[i].radius = 0.005f;
    }

    glDeleteBuffers(1, &vbo_pos);
    glDeleteBuffers(1, &vbo_speed);
    glDeleteBuffers(1, &vbo_quad);
    glDeleteVertexArrays(1, &vao);

    InitRenderData(currentNum);
}

int main() {
    if (!glfwInit()) return -1;
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // Для Mac
    GLFWwindow* window = glfwCreateWindow(800, 600, "Fluid Physics Test", NULL, NULL);
    if (!window) {
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330");

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    std::vector<Particle> particles(numParticles);
    predictedPositions.resize(numParticles);

    GLuint shaderProgram = CreateShaderProgram("../shader.vs", "../shader.fs");

    if (shaderProgram == 0) {
        std::cerr << "Shader program is 0 (failed creation)\n";
        exit(-1);
    }

    InitRenderData(numParticles);

    std::vector<Vec2> renderPos(numParticles);
    std::vector<float> renderSpeeds(numParticles);

    for (int i = 0; i < numParticles; i++) {
        particles[i].x = (i % 12 - 6) * 0.12f;
        particles[i].y = (i / 12) * 0.12f;
        particles[i].vx = particles[i].vy = 0;
    }

    auto lastTime = (float)glfwGetTime();

    while (!glfwWindowShouldClose(window)) {
        float deltaTime = std::min((float)glfwGetTime() - lastTime, 0.016f);
        lastTime = (float)glfwGetTime();

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        ImGui::SetNextWindowSize(ImVec2(350, 0), ImGuiCond_FirstUseEver);

        ImGui::Begin("Simulation Control");
        if (ImGui::Button(isPaused ? "Start" : "Pause")) isPaused = !isPaused;
        static int nextParticleCount = 300;

        ImGui::SliderInt("Particles Count", &nextParticleCount, 10, 2000);

        if (ImGui::IsItemDeactivatedAfterEdit()) {
            numParticles = nextParticleCount;
            ResetSimulation(numParticles, particles, renderPos, renderSpeeds);
        }

        if (ImGui::Button("Reset")) {
            ResetSimulation(numParticles, particles, renderPos, renderSpeeds);
        }

        ImGui::Separator();
        ImGui::Text("Physics Parameters");

        ImGui::SliderFloat("Gravity", &gravity, -20.0f, 20.0f);
        ImGui::SliderFloat("Smoothing Radius", &smoothingRadius, 0.14f, 0.3f);
        ImGui::SliderFloat("Target Density", &targetDensity, 0.1f, 10.0f);
        ImGui::SliderFloat("Pressure Multiplier", &pressureMultiplier, 1.0f, 15.0f);
        ImGui::SliderFloat("Viscosity", &viscosityStrength, 0.0f, 0.05f);
        ImGui::SliderFloat("Damping", &collisionDamping, 0.0f, 1.0f);

        ImGui::Separator();
        ImGui::Text("Mouse Interaction");
        ImGui::SliderFloat("Interaction Radius", &interactionRadius, 0.005f, 1.0f);
        ImGui::SliderFloat("Interaction Strength", &interactionStrength, -200.f, 200.0f);

        ImGui::Separator();
        static float pRadius = 0.005f;
        if (ImGui::SliderFloat("Particle Radius", &pRadius, 0.001f, 0.05f)) {
            for(auto& p : particles) p.radius = pRadius;
        }

        ImGui::End();
        int w, h;
        glfwGetFramebufferSize(window, &w, &h);
        float aspect = (float)w / h;

        if (!isPaused) {
            double mx, my;
            glfwGetCursorPos(window, &mx, &my);
            int width, height;
            glfwGetWindowSize(window, &width, &height);

            float mouseX = ((float)mx / width * 2.0f - 1.0f) * aspect;
            float mouseY = ((float)my / height * 2.0f - 1.0f) * -1.0f;
            Vec2 mousePos = { mouseX, mouseY };

            bool isMousePressed = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
            bool isRightMousePressed = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS;

            if (ImGui::GetIO().WantCaptureMouse) {
                isMousePressed = false;
                isRightMousePressed = false;
            }

#pragma omp parallel for
            for (int i = 0; i < numParticles; i++) {
                particles[i].vy += gravity * deltaTime;
                float predictionStep = 1.0f / 120.0f;
                predictedPositions[i] = {
                        particles[i].x + particles[i].vx * predictionStep,
                        particles[i].y + particles[i].vy * predictionStep
                };
            }

            UpdateSpatialLookup(predictedPositions, smoothingRadius);


#pragma omp parallel for
            for (int i = 0; i < numParticles; i++) {
                particles[i].density = CalculateDensity(i, predictedPositions);
            }// плотность для рассчета давления


#pragma omp parallel for
            for (int i = 0; i < numParticles; i++) {
                Vec2 pressureForce = CalculatePressureForce(i, particles, predictedPositions);
                Vec2 viscForce = CalculateViscosityForce(i, predictedPositions, particles);

                float dens = std::max(0.001f, particles[i].density);

                // применяем давление и viscosity
                particles[i].vx += (pressureForce.x / dens + viscForce.x) * deltaTime;
                particles[i].vy += (pressureForce.y / dens + viscForce.y) * deltaTime;

                if (isMousePressed || isRightMousePressed) {
                    float multiplier = isMousePressed ? 1.0f : -1.0f;
                    Vec2 interactForce = CalculateInteractionForce(mousePos,
                                                                   {particles[i].x, particles[i].y},
                                                                   {particles[i].vx, particles[i].vy});

                    particles[i].vx += interactForce.x * multiplier * deltaTime;
                    particles[i].vy += interactForce.y * multiplier * deltaTime;

                    float dx = mousePos.x - particles[i].x;
                    float dy = mousePos.y - particles[i].y;
                    float dst2 = std::sqrt(dx*dx + dy*dy);
                    if (dst2 < interactionRadius) {
                        float dampFactor = std::pow(0.85f, deltaTime * 60.0f);
                        particles[i].vx *= dampFactor;
                        particles[i].vy *= dampFactor;
                    }
                }
            }

            //это не работает!! чтоб частички не разлетались
            static bool wasMousePressed = false;
            if (wasMousePressed && !isMousePressed) {

                for (int i = 0; i < numParticles; i++) {
                    float dx = mousePos.x - particles[i].x;
                    float dy = mousePos.y - particles[i].y;
                    float dst = std::sqrt(dx*dx + dy*dy);
                    if (dst < interactionRadius) {
                        particles[i].vx *= 0.05f;
                        particles[i].vy *= 0.05f;
                    }
                }
            }
            wasMousePressed = isMousePressed;


// двигаем частицы
            for (int i = 0; i < numParticles; i++) {
                particles[i].x += particles[i].vx * deltaTime;
                particles[i].y += particles[i].vy * deltaTime;
                ResolveCollisions(particles[i], aspect);

                renderPos[i] = {particles[i].x, particles[i].y};
                renderSpeeds[i] = std::sqrt(particles[i].vx * particles[i].vx + particles[i].vy * particles[i].vy);
            }
        }

        glClear(GL_COLOR_BUFFER_BIT);

        glBindBuffer(GL_ARRAY_BUFFER, vbo_pos);
        glBufferSubData(GL_ARRAY_BUFFER, 0, numParticles * sizeof(Vec2), renderPos.data());

        glBindBuffer(GL_ARRAY_BUFFER, vbo_speed);
        glBufferSubData(GL_ARRAY_BUFFER, 0, numParticles * sizeof(float), renderSpeeds.data());

        glUseProgram(shaderProgram);

        GLint aspectLoc = glGetUniformLocation(shaderProgram, "aspect");
        GLint radiusLoc = glGetUniformLocation(shaderProgram, "radius");

        glUniform1f(aspectLoc, aspect);
        glUniform1f(radiusLoc, pRadius);


        glBindVertexArray(vao);
        glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, numParticles);

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
        glfwPollEvents();

    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwTerminate();
    return 0;
}