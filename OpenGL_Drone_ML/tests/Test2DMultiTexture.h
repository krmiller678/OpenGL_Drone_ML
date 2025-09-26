#pragma once

#include "Test.h"

#include "VertexBuffer.h"
#include "VertexBufferLayout.h"
#include "Texture.h"

#include <memory>
/*
| Method       | Think of it as…                        | Accuracy                                     |
| ------------ | -------------------------------------- | -------------------------------------------- |
| **Callback** | OS interrupts you with an “event”      | Exact timing; can catch short presses        |
| **Polling**  | Asking “is this key down?” every frame | Continuous state; can miss very fast presses |
*/

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
        std::unique_ptr<VertexArray> m_VAO_MapElements;
        std::unique_ptr<VertexBuffer> m_VertexBuffer_MapElements;
        std::unique_ptr<IndexBuffer> m_IndexBuffer_MapElements;

        std::unique_ptr<VertexArray> m_VAO_ScreenElements;
        std::unique_ptr<VertexBuffer> m_VertexBuffer_ScreenElements;
        std::unique_ptr<IndexBuffer> m_IndexBuffer_ScreenElements;

        std::unique_ptr<VertexArray> m_VAO_PickupZones;
        std::unique_ptr<VertexBuffer> m_VertexBuffer_PickupZones;
        std::unique_ptr<IndexBuffer> m_IndexBuffer_PickupZones;

        std::unique_ptr<VertexArray> m_VAO_Drone;
        std::unique_ptr<VertexBuffer> m_VertexBuffer_Drone;
        std::unique_ptr<IndexBuffer> m_IndexBuffer_Drone;

        std::unique_ptr<Shader> m_Shader;
        std::unique_ptr<Texture> m_Texture;
        std::unique_ptr<Texture> m_Texture2;
        std::unique_ptr<Texture> m_Texture3;

        // transformation data
        glm::mat4 m_Proj, m_View, m_FreeLook;
        glm::mat4 *m_ViewToUse;
        glm::vec3 m_TranslationA;

        // user input data
        GLFWwindow *m_Window;
        bool m_FreeLookEnabled;
        bool m_LeftClick;
        bool m_CommsOn;
        float m_LastX, m_LastY;

        // positions to display as icons
        std::vector<glm::vec3> m_Targets;

        void CommunicateWithServer();
        void ProcessInput(); // handles per frame polling
        // KeyCallback does not need to be polled it is handled by GLFW directly
        static void KeyCallback(GLFWwindow *window, int key, int scancode, int action, int mods);
        static void MouseCallback(GLFWwindow *window, double xposIn, double yposIn);
        static void MouseButtonCallback(GLFWwindow *window, int button, int action, int mods);

        bool emergencyStop = false;
        bool first_loop = true;
    };

}