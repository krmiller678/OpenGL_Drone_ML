#pragma once

#include "Test.h"

#include <memory>
#include <thread>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <nlohmann/json.hpp>
/*
| Method       | Think of it as…                        | Accuracy                                     |
| ------------ | -------------------------------------- | -------------------------------------------- |
| **Callback** | OS interrupts you with an “event”      | Exact timing; can catch short presses        |
| **Polling**  | Asking “is this key down?” every frame | Continuous state; can miss very fast presses |
*/

namespace test
{

    class Test3DSurvey : public Test
    {
    public:
        Test3DSurvey(GLFWwindow *window);
        ~Test3DSurvey();

        void OnUpdate(float deltaTime) override;
        void OnRender() override;
        void OnImGuiRender() override;

    private:
        // draw call data
        std::unique_ptr<VertexArray> m_VAO_MapElements;
        std::unique_ptr<VertexBuffer> m_VertexBuffer_MapElements;
        std::unique_ptr<IndexBuffer> m_IndexBuffer_MapElements;

        std::unique_ptr<VertexArray> m_VAO_Drone;
        std::unique_ptr<VertexBuffer> m_VertexBuffer_Drone;
        std::unique_ptr<IndexBuffer> m_IndexBuffer_Drone;

        std::unique_ptr<Shader> m_Shader;
        std::unique_ptr<Texture> m_Texture;

        // transformation data
        glm::mat4 m_Proj, m_View;
        glm::vec3 m_Drone, m_TargetTranslation;
        // --- For motion and tilt ---
        glm::vec3 m_LastDronePos = glm::vec3(0.0f);
        glm::vec3 m_Velocity     = glm::vec3(0.0f);

        // Smoothed tilt angles
        float m_CurrentRoll  = 0.0f;
        float m_CurrentPitch = 0.0f;


        // camera work - will need to move into a class
        glm::vec3 m_CameraPos   = glm::vec3(0.0f, 100.0f, 100.0f);
        glm::vec3 m_CameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
        glm::vec3 m_CameraUp    = glm::vec3(0.0f, 1.0f,  0.0f);
        float yaw = -90.0f;
        float pitch = 0.0f;
        float fov = 45.0f;

        // user input data
        GLFWwindow *m_Window;
        bool m_FreeLookEnabled;
        bool m_LeftClick;
        bool m_CommsOn;
        float m_LastX, m_LastY;
        bool emergencyStop = false;

        // Server thread stuff
        void ServerThreadFunc();
        nlohmann::json BuildPayload();
        std::vector<std::vector<float>> LidarScanBelow();
        bool RayIntersectsTriangle(const glm::vec3& orig, const glm::vec3& dir, const Triangle& tri, float& outT) ;
        std::vector<Triangle> m_Terrain;
        bool first_loop = true;
        std::thread m_ServerThread;
        std::queue<nlohmann::json> m_ServerResponses;
        std::mutex m_QueueMutex;
        std::condition_variable m_cv;
        bool stopThread = false;
        
        // handles per frame polling
        void ProcessInput(float deltaTime);
        // KeyCallback does not need to be polled it is handled by GLFW directly
        static void KeyCallback(GLFWwindow *window, int key, int scancode, int action, int mods);
        static void MouseCallback(GLFWwindow *window, double xposIn, double yposIn);
        static void ScrollCallback(GLFWwindow *window, double xoffset, double yoffset);

        // lighting and model rendering
        bool LoadModel(const std::string& path, std::vector<Vertex>& outVertices, 
            std::vector<unsigned int>& outIndices, float rotation, const glm::vec3& position = {100.0f, 100.0f, -200.0f},
            const glm::vec3& scale = {2.0f, 2.0f, 2.0f}, std::vector<Triangle>* terrain = nullptr);
        
    };

}