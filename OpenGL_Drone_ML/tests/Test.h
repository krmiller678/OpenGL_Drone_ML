#pragma once

#include <vector>
#include <functional>
#include <iostream>
#include <string>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include "imgui.h"
#include "imgui_impl_glfw.h"

namespace test {

    class Test
    {
        public:
            Test() {}
            virtual ~Test() {} // not = 0 (pure virtual) so we do not have to implement

            virtual void OnUpdate(float deltaTime) {}
            virtual void OnRender() {}
            virtual void OnImGuiRender() {}
    };

    class TestMenu : public Test
    {
        public:
            TestMenu(Test*& currentTestPointer);

            void OnImGuiRender() override;

            template<typename T>
            void RegisterTest(const std::string& name)
            {
                std::cout << "Registering test " << name << std::endl;

                m_Tests.push_back(std::make_pair(name, []() { return new T(); }));
            }
            template<typename T>
            void RegisterTest(const std::string& name, GLFWwindow *window)
            {
                std::cout << "Registering test with input enabled: " << name << std::endl;

                m_Tests.push_back(std::make_pair(name, [window]() { return new T(window); }));
            }

            // Callback defaults to pass to ImGui
            static void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
            {
                ImGui_ImplGlfw_KeyCallback(window, key, scancode, action, mods);
            }
            static void MouseCallback(GLFWwindow* window, double xpos, double ypos)
            {
                ImGui_ImplGlfw_CursorPosCallback(window, xpos, ypos);
            }
            static void MouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
            {
                ImGui_ImplGlfw_MouseButtonCallback(window, button, action, mods);
            }
            
        private:
            Test*& m_CurrentTest;
            std::vector<std::pair<std::string, std::function<Test*()>>> m_Tests;
    };

}