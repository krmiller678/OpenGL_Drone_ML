#include "Test2DMultiTexture.h"

#include "Renderer.h"

#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"

#include <cpr/cpr.h>
#include <nlohmann/json.hpp>
#include <iostream>

namespace test
{

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

        // target positions
        m_Targets.push_back({480, 135, 0});
        m_Targets.push_back({960, 135, 0});
        m_Targets.push_back({1440, 135, 0});
        m_Targets.push_back({480, 405, 0});

        float positions[] = {
            // positions             // colors           // texture coords
            -50.0f, -50.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, // 0
            50.0f, -50.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f,  // 1
            50.0f, 50.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f,   // 2
            -50.0f, 50.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f   // 3
        };

        unsigned int indices[] = {
            0, 1, 2,
            2, 3, 0};

        GLCall(glEnable(GL_BLEND));
        GLCall(glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA));

        m_VAO = std::make_unique<VertexArray>();

        m_VertexBuffer = std::make_unique<VertexBuffer>(positions, 4 * 8 * sizeof(float));
        VertexBufferLayout layout;
        layout.Push<float>(3);
        layout.Push<float>(3); // 3 floats for color
        layout.Push<float>(2); // Added 2 more floats for each vertex texture coordinates
        m_VAO->AddBuffer(*m_VertexBuffer, layout);

        m_IndexBuffer = std::make_unique<IndexBuffer>(indices, 6);

        m_Shader = std::make_unique<Shader>("res/shaders/Basic2.shader");
        m_Shader->Bind();

        m_Texture = std::make_unique<Texture>("res/textures/alien.png");
        m_Texture2 = std::make_unique<Texture>("res/textures/casa.png");
        m_Texture3 = std::make_unique<Texture>("res/textures/Em_button.png");
        // m_Shader->SetUniform1i("u_Texture", 0); // Texture is bound to slot 0
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
        for (auto &pos : m_Targets)
        {
            glm::mat4 model = glm::translate(glm::mat4(1.0f), pos);
            glm::mat4 mvp = m_Proj * *m_ViewToUse * model;
            m_Shader->Bind();
            m_Shader->SetUniform1i("u_Texture", 1);
            m_Shader->SetUniformMat4f("u_MVP", mvp);
            renderer.Draw(*m_VAO, *m_IndexBuffer, *m_Shader);
        }
        {
            glm::mat4 model = glm::translate(glm::mat4(1.0f), m_TranslationA);
            glm::mat4 mvp = m_Proj * *m_ViewToUse * model; // OpenGL col major leads to this order**
            m_Shader->Bind();
            m_Shader->SetUniform1i("u_Texture", 0);
            m_Shader->SetUniformMat4f("u_MVP", mvp);

            renderer.Draw(*m_VAO, *m_IndexBuffer, *m_Shader);
        }

        {
            glm::vec3 buttonPos(960 - 50, 540 - 50, 0);
            glm::mat4 model = glm::translate(glm::mat4(1.0f), buttonPos);
            glm::mat4 mvp = m_Proj * model; // OpenGL col major leads to this order**
            m_Shader->Bind();
            m_Shader->SetUniform1i("u_Texture", 2); // texture3
            m_Shader->SetUniformMat4f("u_MVP", mvp);
            renderer.Draw(*m_VAO, *m_IndexBuffer, *m_Shader);
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
