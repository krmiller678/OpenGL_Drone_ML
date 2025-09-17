#pragma once

#include "Test.h"

#include "VertexBuffer.h"
#include "VertexBufferLayout.h"
#include "Texture.h"

#include <memory>

namespace test
{

    class Test2DMultiTexture : public Test
    {
    public:
        Test2DMultiTexture(GLFWwindow *window);
        ~Test2DMultiTexture();

        void OnUpdate(float deltaTime) override;
        void OnRender() override;
        void OnImGuiRender() override;

    private:
        // draw call data
        std::unique_ptr<VertexArray> m_VAO;
        std::unique_ptr<VertexBuffer> m_VertexBuffer;
        std::unique_ptr<IndexBuffer> m_IndexBuffer;
        std::unique_ptr<Shader> m_Shader;
        std::unique_ptr<Texture> m_Texture;
        std::unique_ptr<Texture> m_Texture2;

        // transformation data
        glm::mat4 m_Proj, m_View, m_FreeLook;
        glm::mat4* m_ViewToUse;
        glm::vec3 m_TranslationA, m_TranslationB;

        // user input data
        GLFWwindow* m_Window;
        bool m_FreeLookEnabled;
        bool m_LeftClick;
        float m_LastX, m_LastY;

        // positions to display as icons
        std::vector<glm::vec3> m_Targets;

        void CommunicateWithServer();
        void ProcessInput(); // handles per frame polling
        // KeyCallback does not need to be polled it is handled by GLFW directly
        static void KeyCallback(GLFWwindow *window, int key, int scancode, int action, int mods);
        static void MouseCallback(GLFWwindow *window, double xposIn, double yposIn);
    };

}