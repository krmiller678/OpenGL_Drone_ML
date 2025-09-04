cmake_minimum_required(VERSION 3.16)
project(OpenGL_Drone_ML)

set(CMAKE_CXX_STANDARD 17)

include(FetchContent)

# ------------------------
# Dependencies (OpenGL side)
# ------------------------

# GLFW
FetchContent_Declare(
    glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG        3.4
)
FetchContent_MakeAvailable(glfw)

# Set up glew library for access to gpu driver functions
set(GLEW_LIB_DIR "${CMAKE_SOURCE_DIR}/vendor/glew/bin/Release/x64")
file(COPY "${GLEW_LIB_DIR}/glew32.dll" DESTINATION ${CMAKE_BINARY_DIR})

# ImGui FetchContent
FetchContent_Declare(
    imgui_src
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG        v1.91.0
)
FetchContent_MakeAvailable(imgui_src)

# Wrap ImGui as a static library
add_library(ImGuiLib
    ${imgui_src_SOURCE_DIR}/imgui.cpp
    ${imgui_src_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_src_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_src_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_src_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
    ${imgui_src_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
)

# Include ImGui headers
target_include_directories(ImGuiLib PUBLIC
    ${imgui_src_SOURCE_DIR}
    ${imgui_src_SOURCE_DIR}/backends
)

# Make GLFW available to ImGui backend
target_link_libraries(ImGuiLib PUBLIC glfw)

# glm
FetchContent_Declare(
    glm
    GIT_REPOSITORY https://github.com/g-truc/glm.git
    GIT_TAG        1.0.1
)
FetchContent_MakeAvailable(glm)

# ------------------------
# Simulation Side
# ------------------------
add_executable(drone_sim 
                src/Application.cpp
                src/Renderer.cpp
                src/IndexBuffer.cpp
                src/VertexBuffer.cpp
                src/VertexArray.cpp
                src/Shader.cpp
                src/GlError.cpp
                src/Texture.cpp
                vendor/stb_image/stb_image.cpp
                tests/Test.cpp
                tests/TestClearColor.cpp
                tests/TestTexture2D.cpp
    )

# Ensure that resources are copied over
file(COPY "./res" DESTINATION ${CMAKE_BINARY_DIR})

include_directories(
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/tests
    ${CMAKE_SOURCE_DIR}/vendor
    ${CMAKE_SOURCE_DIR}/vendor/glew/include
    )

link_directories(
    ${GLEW_LIB_DIR}
)

target_link_libraries(drone_sim PRIVATE ImGuiLib "${GLEW_LIB_DIR}/glew32.lib" glm opengl32 gdi32 user32 shell32 advapi32)
