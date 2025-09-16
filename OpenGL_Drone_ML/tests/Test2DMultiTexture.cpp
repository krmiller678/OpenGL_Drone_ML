#include "Test2DMultiTexture.h"

#include "Renderer.h"
#include "imgui.h"

#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"

#include <cpr/cpr.h>
#include <nlohmann/json.hpp>
#include <iostream>

namespace test
{

    Test2DMultiTexture::Test2DMultiTexture()
        : m_Proj(glm::ortho(0.0f, 960.0f, 0.0f, 540.0f, -1.0f, 1.0f)),
          m_View(glm::translate(glm::mat4(1.0f), glm::vec3(0, 0, 0))),
          m_TranslationA(200, 200, 0)
    {
        // target positions
        m_Targets.push_back({480, 135, 0});
        m_Targets.push_back({960, 135, 0});
        m_Targets.push_back({1440, 135, 0});
        m_Targets.push_back({480, 405, 0});

        float positions[] = {
            // positions             // colors           // texture coords
            -50.0f, -50.0f,  1.0f,   0.0f, 0.0f, 0.0f,   0.0f, 0.0f, // 0
             50.0f, -50.0f,  1.0f,   0.0f, 0.0f, 0.0f,   1.0f, 0.0f,  // 1
             50.0f,  50.0f,  1.0f,   0.0f, 0.0f, 0.0f,   1.0f, 1.0f,   // 2
            -50.0f,  50.0f,  1.0f,   0.0f, 0.0f, 0.0f,   0.0f, 1.0f   // 3
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
        m_Shader->SetUniform1i("u_Texture", 0); // Texture is bound to slot 0
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

        m_Texture->Bind();
        m_Texture2->Bind(1);

        CommunicateWithServer();

        
        // draw targets
        for (auto &pos : m_Targets)
        {
            glm::mat4 model = glm::translate(glm::mat4(1.0f), pos);
            glm::mat4 mvp = m_Proj * m_View * model;
            m_Shader->Bind();
            m_Shader->SetUniform1i("u_Texture", 1);
            m_Shader->SetUniformMat4f("u_MVP", mvp);
            renderer.Draw(*m_VAO, *m_IndexBuffer, *m_Shader);
        }
        {
            glm::mat4 model = glm::translate(glm::mat4(1.0f), m_TranslationA);
            glm::mat4 mvp = m_Proj * m_View * model; // OpenGL col major leads to this order**
            m_Shader->Bind();
            m_Shader->SetUniform1i("u_Texture", 0);
            m_Shader->SetUniformMat4f("u_MVP", mvp);

            renderer.Draw(*m_VAO, *m_IndexBuffer, *m_Shader);
        }
        /*
                {
                    glm::mat4 model = glm::translate(glm::mat4(1.0f), m_TranslationB);
                    glm::mat4 mvp = m_Proj * m_View * model; // OpenGL col major leads to this order**
                    m_Shader->Bind();
                    m_Shader->SetUniformMat4f("u_MVP", mvp);

                    renderer.Draw(*m_VAO, *m_IndexBuffer, *m_Shader);
                }*/
    }

    void Test2DMultiTexture::OnImGuiRender()
    {
        ImGuiIO &io = ImGui::GetIO();
        (void)io;
        ImGui::SliderFloat3("Translation B", &m_TranslationB.x, 0.0f, 960.0f);
        ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
    }

    void Test2DMultiTexture::CommunicateWithServer()
    {
        nlohmann::json payload;
        payload["test"] = "Test2DMultiTexture"; // <-- identifier, tells server what to do!
        payload["current"] = {{"x", m_TranslationA.x}, {"y", m_TranslationA.y}, {"z", m_TranslationA.z}};
        payload["targets"] = nlohmann::json::array();
        for (auto &t : m_Targets)
            payload["targets"].push_back({{"x", t.x}, {"y", t.y}, {"z", t.z}});
        payload["start"] = {{"x", 200}, {"y", 200}, {"z", 0}};

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

}
