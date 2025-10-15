#include "Test3DB.h"
#include "Renderer.h"

#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"

#include <cpr/cpr.h>
#include <iostream>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assimp/matrix4x4.h>

namespace test
{

    Test3DB::Test3DB(GLFWwindow *window)
        : m_Ortho(glm::ortho(0.0f, 960.0f, 0.0f, 540.0f, -1.0f, 1.0f)),
          m_Proj(glm::perspective(glm::radians(45.0f), 16.0f/9.0f, 0.1f, 1201.0f)),
          m_Drone(200, 200, 0), m_LastX(960 / 2), m_LastY(540 / 2),
          m_Window(window), m_FreeLookEnabled(false), m_LeftClick(false), m_TargetTranslation(200, 200, 0)
    {
        // attaches class instance to the window -> must be used for key callbacks to work!
        glfwSetWindowUserPointer(window, this);
        glfwSetKeyCallback(window, KeyCallback);
        glfwSetCursorPosCallback(window, MouseCallback);
        glfwSetMouseButtonCallback(window, MouseButtonCallback);
        glfwSetScrollCallback(window, ScrollCallback);
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);

        GLCall(glEnable(GL_BLEND));
        GLCall(glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA));
        GLCall(glEnable(GL_DEPTH_TEST));


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
        PushCube(positionsMapElements, indicesMapElements, 600.0f, 1.0f, -500.2f, 600.0f, 2.0f, 500.0f, {0.0, 0.0, 0.0}, 1.0f, &m_Terrain);
        for (unsigned int i = 0; i < 5; i++)
        {
            LoadModel("res/assets/House.obj", positionsMapElements, indicesMapElements, 180.0f, {50.0f, 7.5f, (i*-200.0f - 50.0f)}, {8.0f, 8.0f, 8.0f});
        }
        for (unsigned int i = 0; i < 5; i++)
        {
            LoadModel("res/assets/House.obj", positionsMapElements, indicesMapElements, 90.0f, {(i*200.0f + 250.0f), 7.5f, -900.0f}, {8.0f, 8.0f, 8.0f});
        }
        LoadModel("res/assets/mount1.obj", positionsMapElements, indicesMapElements, 45.0f, {650.0f, 0.0f, -400.0f}, {28.0f, 28.0f, 28.0f}, &m_Terrain);
        
        m_VAO_MapElements = std::make_unique<VertexArray>();

        m_VertexBuffer_MapElements = std::make_unique<VertexBuffer>(positionsMapElements.data(), positionsMapElements.size() * sizeof(Vertex));
        VertexBufferLayout layoutMap;
        layoutMap.Push<float>(3); layoutMap.Push<float>(3); layoutMap.Push<float>(2); layoutMap.Push<float>(1);
        m_VAO_MapElements->AddBuffer(*m_VertexBuffer_MapElements, layoutMap);

        m_IndexBuffer_MapElements = std::make_unique<IndexBuffer>(indicesMapElements.data(), indicesMapElements.size());

        // Pickup Zones - DYNAMIC
        m_VAO_PickupZones = std::make_unique<VertexArray>();
        m_VertexBuffer_PickupZones = std::make_unique<VertexBuffer>(200 * sizeof(Vertex) * 6); // up to 50 drop points reserved
        VertexBufferLayout layoutPickupZones;
        layoutPickupZones.Push<float>(3); layoutPickupZones.Push<float>(3); layoutPickupZones.Push<float>(2); layoutPickupZones.Push<float>(1);
        m_VAO_PickupZones->AddBuffer(*m_VertexBuffer_PickupZones, layoutPickupZones);

        m_IndexBuffer_PickupZones = std::make_unique<IndexBuffer>(300*6); // up to 50 drop points

        // Drone
        std::vector<Vertex> positionsDrone;
        std::vector<unsigned int> indicesDrone;
        LoadModel("res/assets/drone_costum.obj", positionsDrone, indicesDrone, 0.0f, {0.0f, 0.0f, 0.0f}, {5.0f, 5.0f, 5.0f});

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
        m_Texture2 = std::make_unique<Texture>("res/textures/touch_grass.png");
        m_Texture3 = std::make_unique<Texture>("res/textures/Em_button.png");

        m_Texture->Bind();
        m_Texture2->Bind(1);
        m_Texture3->Bind(2);

        m_CameraFront = m_Drone - m_CameraPos;
        m_View = glm::lookAt(m_CameraPos + m_Drone, 
  		                     m_CameraFront + m_CameraPos, 
  		                     m_CameraUp);
    }

    Test3DB::~Test3DB()
    {
        stopThread = true;
        if (m_ServerThread.joinable())
            {m_ServerThread.join(); std::cout << "Thread destroyed!";}
        GLCall(glDisable(GL_DEPTH_TEST));
    }

    void Test3DB::OnUpdate(float deltaTime)
    {
        // set dynamic vertex buffer for PickupZones pre comms with server
        if (m_MakeThread)
        {
            std::vector<Vertex> positionsPickupZones;
            std::vector<unsigned int> indicesPickupZones;
            for (auto &pos : m_Targets)
            {
                PushCube(positionsPickupZones, indicesPickupZones, pos.x, pos.y, pos.z, 10.0f, 10.0f, 10.0f, {0.59f, 0.29f, 0.0f}, -1.0f);
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
                m_TargetTranslation.z = action["z"];
            }
            // --- smooth interpolation each frame ---
            float smoothing = 4.0f; // tweak: higher = snappier
            m_Drone += (m_TargetTranslation - m_Drone) * smoothing * deltaTime;
        }

        ProcessInput(deltaTime);
    }

    void Test3DB::OnRender()
    {
        GLCall(glClearColor(0.0f, 0.0f, 0.0f, 1.0f));
        GLCall(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));

        Renderer renderer;

        if (m_FreeLookEnabled)
        {
            m_View = glm::lookAt(m_CameraPos, 
  		        m_CameraFront + m_CameraPos, 
  		        m_CameraUp);
        }
        else
        {
            m_CameraFront = m_Drone - m_CameraPos;
            m_View = glm::lookAt(m_CameraPos + m_Drone, 
  		        m_CameraFront + m_CameraPos, 
  		        m_CameraUp);
        }
        

        glm::mat4 vp = m_Proj * m_View;

        {
            // Map Elements
            m_Shader->Bind();
            m_Shader->SetUniformMat4f("u_MVP", vp);
            renderer.Draw(*m_VAO_MapElements, *m_IndexBuffer_MapElements, *m_Shader);
        }
        {
            // Screen Elements
            m_Shader->Bind();
            m_Shader->SetUniformMat4f("u_MVP", m_Ortho);
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
            glm::mat4 model = glm::translate(glm::mat4(1.0f), m_Drone);
            glm::mat4 mvp = vp * model;
            m_Shader->Bind();
            m_Shader->SetUniformMat4f("u_MVP", mvp);

            renderer.Draw(*m_VAO_Drone, *m_IndexBuffer_Drone, *m_Shader);
        }
        
    }

    void Test3DB::OnImGuiRender()
    {
        ImGuiIO &io = ImGui::GetIO();
        (void)io;
        ImGui::SliderFloat3("m_Drone", &m_Drone.x, 0.0f, 960.0f);
        ImGui::SliderFloat3("m_CameraPos", &m_CameraPos.x, 0.0f, 960.0f);
        ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
    }

    void Test3DB::ServerThreadFunc() {
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

    nlohmann::json Test3DB::BuildPayload()
    {
        nlohmann::json payload;
        if (first_loop)
        {
            payload["test"] = "3DA"; // <-- identifier, tells server what to do!
            payload["current"] = {{"x", m_Drone.x}, {"y", m_Drone.y}, {"z", m_Drone.z}};
            payload["targets"] = nlohmann::json::array();
            payload["emergency_stop"] = emergencyStop;
            for (auto &t : m_Targets)
                payload["targets"].push_back({{"x", t.x}, {"y", t.y}, {"z", t.z}});
            first_loop = false;
        }
        else
        {
            payload["current"] = {{"x", m_Drone.x}, {"y", m_Drone.y}, {"z", m_Drone.z}};
            payload["emergency_stop"] = emergencyStop;
            payload["lidar_below_drone"] = LidarScanBelow();
        }

        return payload;
    }

    std::vector<std::vector<float>> Test3DB::LidarScanBelow()
    {
        const int gridSize = 5;     // 5x5 samples under drone
        const float spacing = 25.0f; // world units between samples

        std::vector<std::vector<float>> grid(gridSize, std::vector<float>(gridSize, -1.0f));

        glm::vec3 dronePos = m_Drone;
        glm::vec3 rayDir(0, -1, 0); // downwards

        float half = (gridSize - 1) / 2.0f;

        for (int i = 0; i < gridSize; i++) {
            for (int j = 0; j < gridSize; j++) {
                float offsetX = (i - half) * spacing;
                float offsetZ = (j - half) * spacing;

                glm::vec3 origin = dronePos + glm::vec3(offsetX, offsetZ, 0);

                float closestY = -FLT_MAX;
                for (const auto& tri : m_Terrain) {
                    float t;
                    if (RayIntersectsTriangle(origin, rayDir, tri, t)) {
                        float yHit = origin.y + t * rayDir.y;
                        if (yHit < dronePos.y && yHit > closestY) {
                            closestY = yHit;
                        }
                    }
                }

                if (closestY > -FLT_MAX) {
                    grid[i][j] = closestY;
                } else {
                    grid[i][j] = -999.0f; // no ground detected
                }
            }
        }

        return grid;
    }

    bool Test3DB::RayIntersectsTriangle(const glm::vec3& orig, const glm::vec3& dir, const Triangle& tri, float& outT) 
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

    void Test3DB::ProcessInput(float deltaTime)
    {
        if (glfwGetMouseButton(m_Window, GLFW_MOUSE_BUTTON_1) == GLFW_PRESS)
            m_LeftClick = true;
        else
            m_LeftClick = false;
        const float cameraSpeed = 150.0f * deltaTime; // adjust accordingly
        if (m_FreeLookEnabled && glfwGetKey(m_Window, GLFW_KEY_W) == GLFW_PRESS)
            m_CameraPos += cameraSpeed * m_CameraFront;
        if (m_FreeLookEnabled && glfwGetKey(m_Window, GLFW_KEY_S) == GLFW_PRESS)
            m_CameraPos -= cameraSpeed * m_CameraFront;
        if (m_FreeLookEnabled && glfwGetKey(m_Window, GLFW_KEY_A) == GLFW_PRESS)
            m_CameraPos -= glm::normalize(glm::cross(m_CameraFront, m_CameraUp)) * cameraSpeed;
        if (m_FreeLookEnabled && glfwGetKey(m_Window, GLFW_KEY_D) == GLFW_PRESS)
            m_CameraPos += glm::normalize(glm::cross(m_CameraFront, m_CameraUp)) * cameraSpeed;
    }

    void Test3DB::KeyCallback(GLFWwindow *window, int key, int scancode, int action, int mods)
    {
        ImGui_ImplGlfw_KeyCallback(window, key, scancode, action, mods);
        // Grab the instance pointer back out
        auto *self = static_cast<Test3DB *>(glfwGetWindowUserPointer(window));
        if (!self)
            return;

        if (key == GLFW_KEY_F && action == GLFW_RELEASE)
        {
            if (!self->m_FreeLookEnabled)
            {
                self->m_CameraPos = self->m_CameraPos + self->m_Drone;
                self->yaw = -90.0f;
                self->pitch = -45.0f;
                glm::vec3 front;
                front.x = cos(glm::radians(self->yaw)) * cos(glm::radians(self->pitch));
                front.y = sin(glm::radians(self->pitch));
                front.z = sin(glm::radians(self->yaw)) * cos(glm::radians(self->pitch));
                self->m_CameraFront = glm::normalize(front);
            }
            else
                self->m_CameraPos = glm::vec3(0.0f, 100.0f, 100.0f);
            self->m_FreeLookEnabled = !self->m_FreeLookEnabled;
        }

        if (key == GLFW_KEY_SPACE && action == GLFW_RELEASE)
        {
            if (self->m_MakeThread)
            {
                self->m_ServerThread = std::thread(&Test3DB::ServerThreadFunc, self);
                self->m_MakeThread = false; // only make one thread
            }
        }
    }

    void Test3DB::MouseCallback(GLFWwindow *window, double xposIn, double yposIn)
    {
        ImGui_ImplGlfw_CursorPosCallback(window, xposIn, yposIn);

        auto *self = static_cast<Test3DB *>(glfwGetWindowUserPointer(window));
        if (!self)
            return;

        float xpos = static_cast<float>(xposIn);
        float ypos = static_cast<float>(yposIn);

        if (self->m_LeftClick && self->m_FreeLookEnabled)
        {
            float xoffset = xpos - self->m_LastX;
            float yoffset = self->m_LastY - ypos; // inverted Y

            self->m_LastX = xpos;
            self->m_LastY = ypos;

            float sensitivity = 0.5f;
            xoffset *= sensitivity;
            yoffset *= sensitivity;

            self->yaw += xoffset;
            self->pitch += yoffset;
        
            // make sure that when pitch is out of bounds, screen doesn't get flipped
            if (self->pitch > 89.0f)
                self->pitch = 89.0f;
            if (self->pitch < -89.0f)
                self->pitch = -89.0f;

            glm::vec3 front;
            front.x = cos(glm::radians(self->yaw)) * cos(glm::radians(self->pitch));
            front.y = sin(glm::radians(self->pitch));
            front.z = sin(glm::radians(self->yaw)) * cos(glm::radians(self->pitch));
            self->m_CameraFront = glm::normalize(front);
        }
        else
        {
            // update last position so next drag doesn't jump
            self->m_LastX = xpos;
            self->m_LastY = ypos;
        }
    }

    void Test3DB::MouseButtonCallback(GLFWwindow *window, int button, int action, int mods)
    {
        ImGui_ImplGlfw_MouseButtonCallback(window, button, action, mods);

        auto *self = static_cast<Test3DB *>(glfwGetWindowUserPointer(window));
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
                    
            // Step 1: Convert to Normalized Device Coordinates (range [-1, 1])
            float ndcX = (2.0f * xpos) / width - 1.0f;
            float ndcY = 1.0f - (2.0f * ypos) / height;
                    
            // Step 2: Create clip-space position for the near plane (z = -1)
            glm::vec4 clipCoords(ndcX, ndcY, -1.0f, 1.0f);
                    
            // Step 3: Transform to view space
            glm::mat4 invProj = glm::inverse(self->m_Proj);
            glm::vec4 viewCoords = invProj * clipCoords;
            viewCoords = glm::vec4(viewCoords.x, viewCoords.y, -1.0f, 0.0f); 
            // set w=0 so translation doesn’t affect direction
                    
            // Step 4: Transform to world space
            glm::mat4 invView = glm::inverse(self->m_View);
            glm::vec4 worldDir4 = invView * viewCoords;
            glm::vec3 rayDir = glm::normalize(glm::vec3(worldDir4));
                    
            // Step 5: Cast ray
            glm::vec3 rayOrigin = self->m_CameraPos;
            glm::vec3 closestHit;
            float minDist = std::numeric_limits<float>::max();
                    
            for (const auto& tri : self->m_Terrain) {
                float t;
                if (self->RayIntersectsTriangle(rayOrigin, rayDir, tri, t)) 
                {
                    glm::vec3 hit = rayOrigin + t * rayDir;
                    float dist = glm::length(hit - rayOrigin);
                    if (dist < minDist) {
                        minDist = dist;
                        closestHit = hit;
                    }
                }
}

if (minDist < std::numeric_limits<float>::max())
    self->m_Targets.push_back(closestHit);

        }
    }

    void Test3DB::ScrollCallback(GLFWwindow *window, double xoffset, double yoffset)
    {
        ImGui_ImplGlfw_ScrollCallback(window, xoffset, yoffset);
        auto *self = static_cast<Test3DB *>(glfwGetWindowUserPointer(window));
        if (!self)
            return;
        self->fov -= (float)yoffset;
        if (self->fov < 1.0f)
            self->fov = 1.0f;
        if (self->fov > 60.0f)
            self->fov = 60.0f;
        self->m_Proj = glm::perspective(glm::radians(self->fov), 16.0f/9.0f, 0.1f, 1001.0f);
    }

    bool Test3DB::LoadModel(
        const std::string& path, std::vector<Vertex>& outVertices, 
            std::vector<unsigned int>& outIndices, float rotation, const glm::vec3& position,
            const glm::vec3& scale, std::vector<Triangle>* terrain)
    {
        Assimp::Importer importer;

        const aiScene* scene = importer.ReadFile(
            path,
            aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_GenNormals);

        if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
        {
            std::cerr << "ERROR::ASSIMP:: " << importer.GetErrorString() << std::endl;
            return false;
        }

        auto ProcessMesh = [&](aiMesh* mesh)
        {
            unsigned int baseIndex = outVertices.size();

            // Set rotation
            glm::mat4 rotMat = glm::rotate(glm::mat4(1.0f), glm::radians(rotation), glm::vec3(0.0f, 1.0f, 0.0f));

            for (unsigned int i = 0; i < mesh->mNumVertices; ++i)
            {
                glm::vec3 pos(mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z);

                // Apply rotation
                glm::vec4 rotatedPos = rotMat * glm::vec4(pos, 1.0f);

                Vertex vertex;

                // Apply scale and translation
                vertex.x = rotatedPos.x * scale.x + position.x;
                vertex.y = rotatedPos.y * scale.y + position.y;
                vertex.z = rotatedPos.z * scale.z + position.z;

                if (mesh->HasNormals())
                {
                    glm::vec3 normal(mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z);
                    glm::vec3 rotatedNormal = glm::mat3(rotMat) * normal; // rotation only
                    vertex.r = (rotatedNormal.x + 1.0f) * 0.5f;
                    vertex.g = (rotatedNormal.y + 1.0f) * 0.5f;
                    vertex.b = (rotatedNormal.z + 1.0f) * 0.5f;
                }
                else
                {
                    vertex.r = vertex.g = vertex.b = 1.0f;
                }

                if (mesh->mTextureCoords[0])
                {
                    vertex.u = mesh->mTextureCoords[0][i].x;
                    vertex.v = mesh->mTextureCoords[0][i].y;
                }
                else
                {
                    vertex.u = vertex.v = 0.0f;
                }

                vertex.texSlot = -1.0f;
                outVertices.push_back(vertex);
            }

            for (unsigned int i = 0; i < mesh->mNumFaces; ++i)
            {
                aiFace face = mesh->mFaces[i];
                if (face.mNumIndices != 3) continue; // skip non-triangular faces
            
                unsigned int i0 = face.mIndices[0] + baseIndex;
                unsigned int i1 = face.mIndices[1] + baseIndex;
                unsigned int i2 = face.mIndices[2] + baseIndex;
            
                outIndices.push_back(i0);
                outIndices.push_back(i1);
                outIndices.push_back(i2);
            
                if (terrain)
                {
                    const Vertex& v0 = outVertices[i0];
                    const Vertex& v1 = outVertices[i1];
                    const Vertex& v2 = outVertices[i2];
                    terrain->push_back({
                        glm::vec3(v0.x, v0.y, v0.z),
                        glm::vec3(v1.x, v1.y, v1.z),
                        glm::vec3(v2.x, v2.y, v2.z)
                    });
                }
            }
        };

        std::function<void(aiNode*)> ProcessNode = [&](aiNode* node)
        {
            for (unsigned int i = 0; i < node->mNumMeshes; ++i)
                ProcessMesh(scene->mMeshes[node->mMeshes[i]]);
            for (unsigned int i = 0; i < node->mNumChildren; ++i)
                ProcessNode(node->mChildren[i]);
        };

        ProcessNode(scene->mRootNode);
        return true;
    }
}