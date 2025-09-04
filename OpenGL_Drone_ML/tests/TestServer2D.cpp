#include "TestServer2D.h"

#include "Renderer.h"
#include "imgui.h"

#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"

#include <cpr/cpr.h>
#include <nlohmann/json.hpp>
#include <iostream>

namespace test {

    TestServer2D::TestServer2D()
        : m_Proj(glm::ortho(0.0f, 960.0f, 0.0f, 540.0f, -1.0f, 1.0f)), 
        m_View(glm::translate(glm::mat4(1.0f), glm::vec3(0, 0, 0))),
        m_TranslationA(200, 200, 0), m_TranslationB(400, 200, 0)
    {

        float positions[] = { 
            -50.0f, -50.0f, 0.0f, 0.0f, // 0
            50.0f, -50.0f, 1.0f, 0.0f,  // 1
            50.0f, 50.0f, 1.0f, 1.0f,   // 2
            -50.0f, 50.0f, 0.0f, 1.0f   // 3
        };

        unsigned int indices[] = {
            0, 1, 2,
            2, 3, 0
        };

        GLCall(glEnable(GL_BLEND));
        GLCall(glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA));

        m_VAO = std::make_unique<VertexArray>();

        m_VertexBuffer = std::make_unique<VertexBuffer>(positions, 4 * 4 * sizeof(float));        
        VertexBufferLayout layout;
        layout.Push<float>(2);
        layout.Push<float>(2); // Added 2 more floats for each vertex texture coordinates
        m_VAO->AddBuffer(*m_VertexBuffer, layout);
        
        m_IndexBuffer = std::make_unique<IndexBuffer>(indices, 6);


        m_Shader = std::make_unique<Shader>("res/shaders/Basic.shader");
        m_Shader->Bind();
        m_Shader->SetUniform4f("u_Color", 0.8f, 0.3f, 0.8f, 1.0f);

        m_Texture = std::make_unique<Texture>("res/textures/cherno.png");
        m_Shader->SetUniform1i("u_Texture", 0); // Texture is bound to slot 0
    }

    TestServer2D::~TestServer2D()
    {
    }

    void TestServer2D::OnUpdate(float deltaTime)
    {
    }

    void TestServer2D::OnRender()
    {
        GLCall(glClearColor(0.0f, 0.0f, 0.0f, 1.0f));
        GLCall(glClear(GL_COLOR_BUFFER_BIT));

        Renderer renderer;

        m_Texture->Bind();

        CommunicateWithServer();

        {
            glm::mat4 model = glm::translate(glm::mat4(1.0f), m_TranslationA);
            glm::mat4 mvp = m_Proj * m_View * model; // OpenGL col major leads to this order**
            m_Shader->Bind();
            m_Shader->SetUniformMat4f("u_MVP", mvp);
            
            renderer.Draw(*m_VAO, *m_IndexBuffer, *m_Shader);
        }

        {
            glm::mat4 model = glm::translate(glm::mat4(1.0f), m_TranslationB);
            glm::mat4 mvp = m_Proj * m_View * model; // OpenGL col major leads to this order**
            m_Shader->Bind();
            m_Shader->SetUniformMat4f("u_MVP", mvp);
            
            renderer.Draw(*m_VAO, *m_IndexBuffer, *m_Shader);
        }
    }

    void TestServer2D::OnImGuiRender()
    {
        ImGuiIO& io = ImGui::GetIO(); (void)io;
        ImGui::SliderFloat3("Translation B", &m_TranslationB.x, 0.0f, 960.0f);
        ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
    }

    void TestServer2D::CommunicateWithServer()
    {
        nlohmann::json state = {
            {"x", m_TranslationA.x},
            {"y", m_TranslationA.y},
            {"z", m_TranslationA.z}
        };

        try 
        {
            // Send POST request
            cpr::Response r = cpr::Post(
                cpr::Url{"http://localhost:5000/compute"},
                cpr::Body{state.dump()},
                cpr::Header{{"Content-Type", "application/json"}}
            );

            if (r.status_code != 200) {
                std::cerr << "Error: HTTP " << r.status_code << std::endl;
            }

            // Parse response JSON
            nlohmann::json action = nlohmann::json::parse(r.text);
            std::cout << "Next action: " << action.dump() << std::endl;

            // Update sim
            m_TranslationA.x = action["x"];
            m_TranslationA.y = action["y"];

        }
        catch (const std::exception& e) 
        {
            std::cerr << "Request failed: " << e.what() << std::endl;
        }

    }

}
