#include "Test.h"
#include "imgui.h"

namespace test {

    void PushQuad(std::vector<Vertex>& vertices, std::vector<unsigned int>& indices,
            float x, float y, float z, float w, float h, float d, glm::vec3 color, float texSlot, std::vector<Triangle>* terrain) 
    {
        unsigned int startIndex = vertices.size();

        if (d == 0) // const z
        {
            vertices.push_back({x - w, y - h, z, color.r, color.g, color.b, 0.0f, 0.0f, texSlot});
            vertices.push_back({x + w, y - h, z, color.r, color.g, color.b, 1.0f, 0.0f, texSlot});
            vertices.push_back({x + w, y + h, z, color.r, color.g, color.b, 1.0f, 1.0f, texSlot});
            vertices.push_back({x - w, y + h, z, color.r, color.g, color.b, 0.0f, 1.0f, texSlot});
        }
        else if (h == 0) // const y
        {
            vertices.push_back({x - w, y, z + d, color.r, color.g, color.b, 0.0f, 0.0f, texSlot});
            vertices.push_back({x + w, y, z + d, color.r, color.g, color.b, 1.0f, 0.0f, texSlot});
            vertices.push_back({x + w, y, z - d, color.r, color.g, color.b, 1.0f, 1.0f, texSlot});
            vertices.push_back({x - w, y, z - d, color.r, color.g, color.b, 0.0f, 1.0f, texSlot});
        }
        else if (w == 0) // const x
        {
            vertices.push_back({x, y - h, z-h, color.r, color.g, color.b, 0.0f, 0.0f, texSlot});
            vertices.push_back({x, y - h, z+h, color.r, color.g, color.b, 1.0f, 0.0f, texSlot});
            vertices.push_back({x, y + h, z+h, color.r, color.g, color.b, 1.0f, 1.0f, texSlot});
            vertices.push_back({x, y + h, z-h, color.r, color.g, color.b, 0.0f, 1.0f, texSlot});
        }

        indices.push_back(startIndex + 0);
        indices.push_back(startIndex + 1);
        indices.push_back(startIndex + 2);
        indices.push_back(startIndex + 2);
        indices.push_back(startIndex + 3);
        indices.push_back(startIndex + 0);

        if (terrain) {
        glm::vec3 v0(vertices[startIndex+0].x, vertices[startIndex+0].y, vertices[startIndex+0].z);
        glm::vec3 v1(vertices[startIndex+1].x, vertices[startIndex+1].y, vertices[startIndex+1].z);
        glm::vec3 v2(vertices[startIndex+2].x, vertices[startIndex+2].y, vertices[startIndex+2].z);
        glm::vec3 v3(vertices[startIndex+3].x, vertices[startIndex+3].y, vertices[startIndex+3].z);

        terrain->push_back({v0, v1, v2});
        terrain->push_back({v2, v3, v0});
        }
    }

    void PushCube(std::vector<Vertex>& vertices, std::vector<unsigned int>& indices,
            float x, float y, float z, float w, float h, float d, glm::vec3 color, float texSlot, std::vector<Triangle>* terrain) 
    {
        PushQuad(vertices, indices, x, y, z-d, w, h, 0.0f, color, texSlot, terrain);
        PushQuad(vertices, indices, x, y, z+d, w, h, 0.0f, color, texSlot, terrain);
        PushQuad(vertices, indices, x-w, y, z, 0.0f, h, d, color, texSlot, terrain);
        PushQuad(vertices, indices, x+w, y, z, 0.0f, h, d, color, texSlot, terrain);
        PushQuad(vertices, indices, x, y-h, z, w, 0.0f, d, color, texSlot, terrain);
        PushQuad(vertices, indices, x, y+h, z, w, 0.0f, d, color, texSlot, terrain);
    }

    TestMenu::TestMenu(Test*& currentTestPointer)
        : m_CurrentTest(currentTestPointer)
    {
    }

    void TestMenu::OnImGuiRender()
    {
        for (auto& test : m_Tests) 
        {
            if (ImGui::Button(test.first.c_str()))
                m_CurrentTest = test.second();
        }
    }
}