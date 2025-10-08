#include "Test2DMultiTexture.h"
#include "Renderer.h"

#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"

#include <cpr/cpr.h>
#include <iostream>

namespace test
{

    Test2DMultiTexture::Test2DMultiTexture(GLFWwindow *window)
        : m_Proj(glm::ortho(0.0f, 960.0f, 0.0f, 540.0f, -1.0f, 1.0f)),
          m_View(glm::translate(glm::mat4(1.0f), glm::vec3(0, 0, 0))),
          m_FreeLook(glm::translate(glm::mat4(1.0f), glm::vec3(0, 0, 0))),
          m_TranslationA(200, 200, 1), m_LastX(960 / 2), m_LastY(540 / 2),
          m_Window(window), m_FreeLookEnabled(false), m_LeftClick(false), m_TargetTranslation(200, 200, 1)
    {
        // attaches class instance to the window -> must be used for key callbacks to work!
        glfwSetWindowUserPointer(window, this);
        glfwSetKeyCallback(window, KeyCallback);
        glfwSetCursorPosCallback(window, MouseCallback);
        glfwSetMouseButtonCallback(window, MouseButtonCallback);
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);

        GLCall(glEnable(GL_BLEND));
        GLCall(glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA));

        // Screen Elements
        std::vector<Vertex> positionsScreenElements;
        std::vector<unsigned int> indicesScreenElements;
        PushQuad(positionsScreenElements, indicesScreenElements, 910.0f, 490.0f, 1.0f, 50.0f, 50.0f, 0.0f, {0.0f, 0.0f, 0.0f}, 2.0f);

        m_VAO_ScreenElements = std::make_unique<VertexArray>();

        m_VertexBuffer_ScreenElements = std::make_unique<VertexBuffer>(positionsScreenElements.data(), positionsScreenElements.size() * sizeof(Vertex));
        VertexBufferLayout layoutScreen;
        layoutScreen.Push<float>(3); layoutScreen.Push<float>(3); layoutScreen.Push<float>(2); layoutScreen.Push<float>(1);
        m_VAO_ScreenElements->AddBuffer(*m_VertexBuffer_ScreenElements, layoutScreen);

        m_IndexBuffer_ScreenElements = std::make_unique<IndexBuffer>(indicesScreenElements.data(), indicesScreenElements.size());

        // Map Elements (Houses / Ground)
        std::vector<Vertex> positionsMapElements;
        std::vector<unsigned int> indicesMapElements;
        PushQuad(positionsMapElements, indicesMapElements, 1000.0f, 500.0f, 0.0f, 1200.0f, 500.0f, 0.0f, {0.0f, 0.5f, 0.0f}, -1.0f, &m_Terrain);
        for (unsigned int i = 0; i < 5; i++)
        {
            PushQuad(positionsMapElements, indicesMapElements, i*500.0f + 150.0f, 150.0f, 0.9f, 50.0f, 50.0f, 0.0f, {0.0f, 0.0f, 0.0f}, 1.0f, &m_Terrain);
        }
        for (unsigned int i = 0; i < 5; i++)
        {
            PushQuad(positionsMapElements, indicesMapElements, i*500.0f + 150.0f, 450.0f, 0.9f, 50.0f, 50.0f, 0.0f, {0.0f, 0.0f, 0.0f}, 1.0f, &m_Terrain);
        }

        for (auto tri : m_Terrain)
        {
            std::cout << tri.v0.z << std::endl << tri.v1.z << std::endl << tri.v2.z << std::endl;
        }

        m_VAO_MapElements = std::make_unique<VertexArray>();

        m_VertexBuffer_MapElements = std::make_unique<VertexBuffer>(positionsMapElements.data(), positionsMapElements.size() * sizeof(Vertex));
        VertexBufferLayout layoutMap;
        layoutMap.Push<float>(3); layoutMap.Push<float>(3); layoutMap.Push<float>(2); layoutMap.Push<float>(1);
        m_VAO_MapElements->AddBuffer(*m_VertexBuffer_MapElements, layoutMap);

        m_IndexBuffer_MapElements = std::make_unique<IndexBuffer>(indicesMapElements.data(), indicesMapElements.size());

        // Pickup Zones - DYNAMIC
        m_VAO_PickupZones = std::make_unique<VertexArray>();
        m_VertexBuffer_PickupZones = std::make_unique<VertexBuffer>(200 * sizeof(Vertex)); // up to 50 drop points reserved
        VertexBufferLayout layoutPickupZones;
        layoutPickupZones.Push<float>(3); layoutPickupZones.Push<float>(3); layoutPickupZones.Push<float>(2); layoutPickupZones.Push<float>(1);
        m_VAO_PickupZones->AddBuffer(*m_VertexBuffer_PickupZones, layoutPickupZones);

        m_IndexBuffer_PickupZones = std::make_unique<IndexBuffer>(300); // up to 50 drop points

        // Drone
        std::vector<Vertex> positionsDrone;
        std::vector<unsigned int> indicesDrone;
        PushQuad(positionsDrone, indicesDrone, 0.0f, 0.0f, 0.0f, 50.0f, 50.0f, 0.0f, {0.0f, 0.0f, 0.0f}, 0.0f);

        m_VAO_Drone = std::make_unique<VertexArray>();

        m_VertexBuffer_Drone = std::make_unique<VertexBuffer>(positionsDrone.data(), positionsDrone.size() * sizeof(Vertex));
        VertexBufferLayout layoutDrone;
        layoutDrone.Push<float>(3); layoutDrone.Push<float>(3); layoutDrone.Push<float>(2); layoutDrone.Push<float>(1);
        m_VAO_Drone->AddBuffer(*m_VertexBuffer_Drone, layoutDrone);

        m_IndexBuffer_Drone = std::make_unique<IndexBuffer>(indicesDrone.data(), indicesDrone.size());

        // Shader and Textures setup
        m_Shader = std::make_unique<Shader>("res/shaders/Basic2.shader");
        m_Shader->Bind();
        int samplers[8] = { 0, 1, 2, 3, 4, 5, 6, 7 }; // allow up to 8 textures
        m_Shader->SetUniform1iv("u_Textures", 8, samplers);

        m_Texture = std::make_unique<Texture>("res/textures/alien.png");
        m_Texture2 = std::make_unique<Texture>("res/textures/casa.png");
        m_Texture3 = std::make_unique<Texture>("res/textures/Em_button.png");

        m_Texture->Bind();
        m_Texture2->Bind(1);
        m_Texture3->Bind(2);

        m_ViewToUse = &m_View;
    }

    Test2DMultiTexture::~Test2DMultiTexture()
    {
        stopThread = true;
        if (m_ServerThread.joinable())
            {m_ServerThread.join(); std::cout << "Thread destroyed!";}
    }

    void Test2DMultiTexture::OnUpdate(float deltaTime)
    {
        // set dynamic vertex buffer for PickupZones pre comms with server
        if (m_MakeThread)
        {
            std::vector<Vertex> positionsPickupZones;
            std::vector<unsigned int> indicesPickupZones;
            for (auto &pos : m_Targets)
            {
                PushQuad(positionsPickupZones, indicesPickupZones, pos.x, pos.y, pos.z, 10.0f, 10.0f, 0.0f, { 0.59f, 0.29f, 0.0f }, -1.0f);
            }

            m_VertexBuffer_PickupZones->Bind();
            GLCall(glBufferSubData(GL_ARRAY_BUFFER, 0, positionsPickupZones.size() * sizeof(Vertex), positionsPickupZones.data()));
            m_IndexBuffer_PickupZones->Bind();
            GLCall(glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, indicesPickupZones.size() * sizeof(unsigned int), indicesPickupZones.data()));
        }
        else
        {
            nlohmann::json action;
            {
                std::unique_lock<std::mutex> lock(m_QueueMutex);
                if (!m_ServerResponses.empty()) 
                {
                    action = m_ServerResponses.front();
                    m_ServerResponses.pop();
                }
            }
            if (!action.empty()) {
                // Update sim
                m_TargetTranslation.x = action["x"];
                m_TargetTranslation.y = action["y"];
            }
            // --- smooth interpolation each frame ---
            float smoothing = 4.0f; // tweak: higher = snappier
            if (m_TargetTranslation.x > 910)
                {
                    m_View = glm::translate(m_View, glm::vec3((m_TranslationA.x - m_TargetTranslation.x ) * smoothing * deltaTime, 0, 0));
                }
            m_TranslationA += (m_TargetTranslation - m_TranslationA) * smoothing * deltaTime;
            std::cout << m_TargetTranslation.x << std::endl;
        }
    }

    void Test2DMultiTexture::OnRender()
    {
        GLCall(glClearColor(0.0f, 0.0f, 0.0f, 1.0f));
        GLCall(glClear(GL_COLOR_BUFFER_BIT));

        Renderer renderer;

        ProcessInput();

        glm::mat4 vp = m_Proj * *m_ViewToUse;
        {
            // Map Elements
            m_Shader->Bind();
            m_Shader->SetUniformMat4f("u_MVP", vp);
            renderer.Draw(*m_VAO_MapElements, *m_IndexBuffer_MapElements, *m_Shader);
        }
        {
            // Screen Elements
            m_Shader->Bind();
            m_Shader->SetUniformMat4f("u_MVP", m_Proj);
            renderer.Draw(*m_VAO_ScreenElements, *m_IndexBuffer_ScreenElements, *m_Shader);
        }
        {
            // Pickup Zones
            m_Shader->Bind();
            m_Shader->SetUniformMat4f("u_MVP", vp);
            renderer.Draw(*m_VAO_PickupZones, *m_IndexBuffer_PickupZones, *m_Shader);
        }
        {
            // Drone
            glm::mat4 model = glm::translate(glm::mat4(1.0f), m_TranslationA);
            glm::mat4 mvp = vp * model; // OpenGL col major leads to this order**
            m_Shader->Bind();
            m_Shader->SetUniformMat4f("u_MVP", mvp);

            renderer.Draw(*m_VAO_Drone, *m_IndexBuffer_Drone, *m_Shader);
        }
        
    }

    void Test2DMultiTexture::OnImGuiRender()
    {
        ImGuiIO &io = ImGui::GetIO();
        (void)io;
        ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
    }

    void Test2DMultiTexture::ServerThreadFunc() {
        std::cout << "Thread created!" << std::endl;
        while (!stopThread) {
            nlohmann::json payload = BuildPayload();
            try {
                // Send POST request
                cpr::Response r = cpr::Post(
                    cpr::Url{"http://localhost:5000/compute"},
                    cpr::Body{payload.dump()},
                    cpr::Header{{"Content-Type", "application/json"}});

                if (r.status_code == 200) {
                    nlohmann::json action = nlohmann::json::parse(r.text);

                    {
                        std::lock_guard<std::mutex> lock(m_QueueMutex);
                        m_ServerResponses.push(action);
                    }
                    m_cv.notify_one(); // probably not needed - leaving for now
                }
            } catch (const std::exception &e) {
                std::cerr << "Request failed: " << e.what() << std::endl;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(250)); // throttle
        }
    }

    nlohmann::json Test2DMultiTexture::BuildPayload()
    {
        nlohmann::json payload;
        if (first_loop)
        {
            payload["test"] = "2DMT"; // <-- identifier, tells server what to do!
            payload["current"] = {{"x", m_TranslationA.x}, {"y", m_TranslationA.y}, {"z", m_TranslationA.z}};
            payload["targets"] = nlohmann::json::array();
            payload["emergency_stop"] = emergencyStop;
            for (auto &t : m_Targets)
                payload["targets"].push_back({{"x", t.x}, {"y", t.y}, {"z", t.z}});
            first_loop = false;
        }
        else
        {
            payload["current"] = {{"x", m_TranslationA.x}, {"y", m_TranslationA.y}, {"z", m_TranslationA.z}};
            payload["emergency_stop"] = emergencyStop;
            payload["lidar_below_drone"] = LidarScanBelow();
        }

        return payload;
    }

    std::vector<std::vector<float>> Test2DMultiTexture::LidarScanBelow()
    {
        const int gridSize = 5;     // 5x5 samples under drone
        const float spacing = 25.0f; // world units between samples

        std::vector<std::vector<float>> grid(gridSize, std::vector<float>(gridSize, -1.0f));

        glm::vec3 dronePos = m_TranslationA;
        glm::vec3 rayDir(0, 0, -1); // downwards

        float half = (gridSize - 1) / 2.0f;

        for (int i = 0; i < gridSize; i++) {
            for (int j = 0; j < gridSize; j++) {
                float offsetX = (i - half) * spacing;
                float offsetY = (j - half) * spacing;

                glm::vec3 origin = dronePos + glm::vec3(offsetX, offsetY, 0);

                float closestZ = -FLT_MAX;
                for (const auto& tri : m_Terrain) {
                    float t;
                    if (RayIntersectsTriangle(origin, rayDir, tri, t)) {
                        float zHit = origin.z + t * rayDir.z;
                        if (zHit < dronePos.z && zHit > closestZ) {
                            closestZ = zHit;
                        }
                    }
                }

                if (closestZ > -FLT_MAX) {
                    grid[i][j] = closestZ;
                } else {
                    grid[i][j] = -2.0f; // no ground detected
                }
            }
        }

        return grid;
    }

    bool Test2DMultiTexture::RayIntersectsTriangle(const glm::vec3& orig, const glm::vec3& dir, const Triangle& tri, float& outT) 
    {
        const float EPSILON = 1e-8f;
        glm::vec3 edge1 = tri.v1 - tri.v0;
        glm::vec3 edge2 = tri.v2 - tri.v0;
        glm::vec3 h = glm::cross(dir, edge2);
        float a = glm::dot(edge1, h);
        if (fabs(a) < EPSILON) return false; // ray parallel

        float f = 1.0f / a;
        glm::vec3 s = orig - tri.v0;
        float u = f * glm::dot(s, h);
        if (u < 0.0f || u > 1.0f) return false;

        glm::vec3 q = glm::cross(s, edge1);
        float v = f * glm::dot(dir, q);
        if (v < 0.0f || u + v > 1.0f) return false;

        float t = f * glm::dot(edge2, q);
        if (t > EPSILON) {
            outT = t;
            return true;
        }
        return false;
    }

    void Test2DMultiTexture::ProcessInput()
    {
        if (glfwGetMouseButton(m_Window, GLFW_MOUSE_BUTTON_1) == GLFW_PRESS)
            m_LeftClick = true;
        else
            m_LeftClick = false;
    }

    void Test2DMultiTexture::KeyCallback(GLFWwindow *window, int key, int scancode, int action, int mods)
    {
        ImGui_ImplGlfw_KeyCallback(window, key, scancode, action, mods);
        // Grab the instance pointer back out
        auto *self = static_cast<Test2DMultiTexture *>(glfwGetWindowUserPointer(window));
        if (!self)
            return;

        if (key == GLFW_KEY_F && action == GLFW_RELEASE)
        {
            if (!self->m_FreeLookEnabled)
                self->m_ViewToUse = &self->m_FreeLook;
            else
                self->m_ViewToUse = &self->m_View;
            self->m_FreeLookEnabled = !self->m_FreeLookEnabled;
        }

        if (key == GLFW_KEY_SPACE && action == GLFW_RELEASE)
        {
            if (self->m_MakeThread)
            {
                self->m_ServerThread = std::thread(&Test2DMultiTexture::ServerThreadFunc, self);
                self->m_MakeThread = false; // only make one thread
            }
        }
    }

    void Test2DMultiTexture::MouseCallback(GLFWwindow *window, double xposIn, double yposIn)
    {
        ImGui_ImplGlfw_CursorPosCallback(window, xposIn, yposIn);

        auto *self = static_cast<Test2DMultiTexture *>(glfwGetWindowUserPointer(window));
        if (!self)
            return;

        float xpos = static_cast<float>(xposIn);
        float ypos = static_cast<float>(yposIn);

        if (self->m_LeftClick)
        {
            float xoffset = xpos - self->m_LastX;
            float yoffset = self->m_LastY - ypos; // inverted Y

            self->m_LastX = xpos;
            self->m_LastY = ypos;

            self->m_FreeLook = glm::translate(self->m_FreeLook, glm::vec3(xoffset, yoffset, 0));
        }
        else
        {
            // update last position so next drag doesn't jump
            self->m_LastX = xpos;
            self->m_LastY = ypos;
        }
    }

    void Test2DMultiTexture::MouseButtonCallback(GLFWwindow *window, int button, int action, int mods)
    {
        ImGui_ImplGlfw_MouseButtonCallback(window, button, action, mods);

        auto *self = static_cast<Test2DMultiTexture *>(glfwGetWindowUserPointer(window));
        if (!self)
            return;

        if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE)
        {
            double xpos, ypos;
            glfwGetCursorPos(window, &xpos, &ypos);

            float buttonX = 910.0f;
            float buttonY = 50.0f;
            float radius = 50.0f;

            // Compute distance from click to button center
            float dx = static_cast<float>(xpos) - buttonX;
            float dy = static_cast<float>(ypos) - buttonY;
            float distSquared = dx * dx + dy * dy;

            if (distSquared <= radius * radius)
            {
                self->emergencyStop = !self->emergencyStop;
                std::cout << "Emergency button clicked!" << std::endl;
            }
        }

        if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_RELEASE)
        {
            double xpos, ypos;
            glfwGetCursorPos(window, &xpos, &ypos);

            int width, height;
            glfwGetWindowSize(window, &width, &height);

            // Step 1: window → NDC
            float ndcX = (2.0f * xpos) / width - 1.0f;
            float ndcY = 1.0f - (2.0f * ypos) / height;

            glm::vec4 clipCoords(ndcX, ndcY, 0.0f, 1.0f);

            // Step 2: clip → world
            glm::mat4 viewProj = self->m_Proj * *(self->m_ViewToUse);
            glm::mat4 invVP = glm::inverse(viewProj);
            glm::vec4 worldPos = invVP * clipCoords;
            worldPos /= worldPos.w;

            // Step 3: store
            self->m_Targets.push_back(glm::vec3(worldPos));
        }
    }

}
