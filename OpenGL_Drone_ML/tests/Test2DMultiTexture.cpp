#include "Test2DMultiTexture.h"

#include "Renderer.h"

#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"

#include <cpr/cpr.h>
#include <nlohmann/json.hpp>
#include <iostream>

namespace test
{

    // Vertex struct to make adding positions easier and help with dynamic vertex buffer
    struct Vertex {
        float x, y, z;
        float r, g, b;
        float u, v;
        float texSlot;
    };

    void PushQuad(std::vector<Vertex>& vertices, std::vector<unsigned int>& indices,
                float x, float y, float z, float w, float h, glm::vec3 color, float texSlot) 
    {
        unsigned int startIndex = vertices.size();

        vertices.push_back({x - w, y - h, z, color.r, color.g, color.b, 0.0f, 0.0f, texSlot});
        vertices.push_back({x + w, y - h, z, color.r, color.g, color.b, 1.0f, 0.0f, texSlot});
        vertices.push_back({x + w, y + h, z, color.r, color.g, color.b, 1.0f, 1.0f, texSlot});
        vertices.push_back({x - w, y + h, z, color.r, color.g, color.b, 0.0f, 1.0f, texSlot});

        indices.push_back(startIndex + 0);
        indices.push_back(startIndex + 1);
        indices.push_back(startIndex + 2);
        indices.push_back(startIndex + 2);
        indices.push_back(startIndex + 3);
        indices.push_back(startIndex + 0);
    }

    Test2DMultiTexture::Test2DMultiTexture(GLFWwindow *window)
        : m_Proj(glm::ortho(0.0f, 960.0f, 0.0f, 540.0f, -1.0f, 1.0f)),
          m_View(glm::translate(glm::mat4(1.0f), glm::vec3(0, 0, 0))),
          m_FreeLook(glm::translate(glm::mat4(1.0f), glm::vec3(0, 0, 0))),
          m_TranslationA(200, 200, 0), m_LastX(960 / 2), m_LastY(540 / 2),
          m_Window(window), m_FreeLookEnabled(false), m_LeftClick(false), m_CommsOn(false)
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
        PushQuad(positionsScreenElements, indicesScreenElements, 910.0f, 490.0f, 1.0f, 50.0f, 50.0f, {0.0f, 0.0f, 0.0f}, 2.0f);

        m_VAO_ScreenElements = std::make_unique<VertexArray>();

        m_VertexBuffer_ScreenElements = std::make_unique<VertexBuffer>(positionsScreenElements.data(), positionsScreenElements.size() * sizeof(Vertex));
        VertexBufferLayout layoutScreen;
        layoutScreen.Push<float>(3); layoutScreen.Push<float>(3); layoutScreen.Push<float>(2); layoutScreen.Push<float>(1);
        m_VAO_ScreenElements->AddBuffer(*m_VertexBuffer_ScreenElements, layoutScreen);

        m_IndexBuffer_ScreenElements = std::make_unique<IndexBuffer>(indicesScreenElements.data(), indicesScreenElements.size());

        // Map Elements (Houses)
        std::vector<Vertex> positionsMapElements;
        std::vector<unsigned int> indicesMapElements;
        for (unsigned int i = 0; i < 5; i++)
        {
            PushQuad(positionsMapElements, indicesMapElements, i*500.0f + 150.0f, 150.0f, 1.0f, 50.0f, 50.0f, {0.0f, 0.0f, 0.0f}, 1.0f);
        }
        for (unsigned int i = 0; i < 5; i++)
        {
            PushQuad(positionsMapElements, indicesMapElements, i*500.0f + 150.0f, 450.0f, 1.0f, 50.0f, 50.0f, {0.0f, 0.0f, 0.0f}, 1.0f);
        }

        m_VAO_MapElements = std::make_unique<VertexArray>();

        m_VertexBuffer_MapElements = std::make_unique<VertexBuffer>(positionsMapElements.data(), positionsMapElements.size() * sizeof(Vertex));
        VertexBufferLayout layoutMap;
        layoutMap.Push<float>(3); layoutMap.Push<float>(3); layoutMap.Push<float>(2); layoutMap.Push<float>(1);
        m_VAO_MapElements->AddBuffer(*m_VertexBuffer_MapElements, layoutMap);

        m_IndexBuffer_MapElements = std::make_unique<IndexBuffer>(indicesMapElements.data(), indicesMapElements.size());

        // Pickup Zones
        m_VAO_PickupZones = std::make_unique<VertexArray>();

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
    }

    void Test2DMultiTexture::OnUpdate(float deltaTime)
    {
        // set dynamic vertex buffer for PickupZones
        //m_VertexBuffer_PickupZones->Bind();
        //glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(targets), targets);
    }

    void Test2DMultiTexture::OnRender()
    {
        GLCall(glClearColor(0.0f, 0.5f, 0.0f, 1.0f));
        GLCall(glClear(GL_COLOR_BUFFER_BIT));

        Renderer renderer;

        ProcessInput();
        if (m_CommsOn)
            CommunicateWithServer();

        // draw targets
        //for (auto &pos : m_Targets)
        //{
        //    glm::mat4 model = glm::translate(glm::mat4(1.0f), pos);
        //    glm::mat4 mvp = m_Proj * *m_ViewToUse * model;
        //    m_Shader->Bind();
        //    m_Shader->SetUniformMat4f("u_MVP", mvp);
        //    renderer.Draw(*m_VAO, *m_IndexBuffer, *m_Shader);
        //}
        //{
        //    glm::mat4 model = glm::translate(glm::mat4(1.0f), m_TranslationA);
        //    glm::mat4 mvp = m_Proj * *m_ViewToUse * model; // OpenGL col major leads to this order**
        //    m_Shader->Bind();
        //    m_Shader->SetUniformMat4f("u_MVP", mvp);
//
        //    renderer.Draw(*m_VAO, *m_IndexBuffer, *m_Shader);
        //}

        {
            m_Shader->Bind();
            m_Shader->SetUniformMat4f("u_MVP", m_Proj);
            renderer.Draw(*m_VAO_ScreenElements, *m_IndexBuffer_ScreenElements, *m_Shader);
        }
        {
            glm::mat4 mvp = m_Proj * *m_ViewToUse;
            m_Shader->Bind();
            m_Shader->SetUniformMat4f("u_MVP", mvp);
            renderer.Draw(*m_VAO_MapElements, *m_IndexBuffer_MapElements, *m_Shader);
        }
    }

    void Test2DMultiTexture::OnImGuiRender()
    {
        ImGuiIO &io = ImGui::GetIO();
        (void)io;
        ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
    }

    void Test2DMultiTexture::CommunicateWithServer()
    {
        nlohmann::json payload;
        if (first_loop)
        {
            payload["test"] = "Test2DMultiTexture"; // <-- identifier, tells server what to do!
            payload["current"] = {{"x", m_TranslationA.x}, {"y", m_TranslationA.y}, {"z", m_TranslationA.z}};
            payload["targets"] = nlohmann::json::array();
            payload["emergency_stop"] = emergencyStop;
            for (auto &t : m_Targets)
                payload["targets"].push_back({{"x", t.x}, {"y", t.y}, {"z", t.z}});
            payload["start"] = {{"x", 200}, {"y", 200}, {"z", 0}};
            first_loop = false;
        }
        else
        {
            payload["current"] = {{"x", m_TranslationA.x}, {"y", m_TranslationA.y}, {"z", m_TranslationA.z}};
            payload["emergency_stop"] = emergencyStop;
        }

        try
        {
            // Send POST request
            cpr::Response r = cpr::Post(
                cpr::Url{"http://localhost:5000/compute"},
                cpr::Body{payload.dump()},
                cpr::Header{{"Content-Type", "application/json"}});

            if (r.status_code != 200)
            {
                std::cerr << "Error: HTTP " << r.status_code << std::endl;
            }

            // Parse response JSON
            nlohmann::json action = nlohmann::json::parse(r.text);
            std::cout << "Next action: " << action.dump() << std::endl;

            // Update sim
            if (action["x"] > 910)
            {
                m_View = glm::translate(m_View, glm::vec3(m_TranslationA.x - float(action["x"]), 0, 0));
            }
            m_TranslationA.x = action["x"];
            m_TranslationA.y = action["y"];
        }
        catch (const std::exception &e)
        {
            std::cerr << "Request failed: " << e.what() << std::endl;
        }
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
            self->m_CommsOn = true;
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
                self->emergencyStop = true;
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
            glm::mat4 viewProj = self->m_Proj * self->m_FreeLook;
            glm::mat4 invVP = glm::inverse(viewProj);
            glm::vec4 worldPos = invVP * clipCoords;
            worldPos /= worldPos.w;

            // Step 3: store
            self->m_Targets.push_back(glm::vec3(worldPos));
        }
    }

}
