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
        Test2DMultiTexture();
        ~Test2DMultiTexture();

        void OnUpdate(float deltaTime) override;
        void OnRender() override;
        void OnImGuiRender() override;

    private:
        std::unique_ptr<VertexArray> m_VAO;
        std::unique_ptr<VertexBuffer> m_VertexBuffer;
        std::unique_ptr<IndexBuffer> m_IndexBuffer;
        std::unique_ptr<Shader> m_Shader;
        std::unique_ptr<Texture> m_Texture;
        std::unique_ptr<Texture> m_Texture2;

        glm::mat4 m_Proj, m_View;
        glm::vec3 m_TranslationA, m_TranslationB;

        // positions to display as icons
        std::vector<glm::vec3> m_Targets;

        void CommunicateWithServer();
    };

}