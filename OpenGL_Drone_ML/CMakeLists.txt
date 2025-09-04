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

# json
FetchContent_Declare(
    json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.11.2
)
FetchContent_MakeAvailable(json)

# http requests
set(CPR_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(CPR_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
    cpr
    GIT_REPOSITORY https://github.com/libcpr/cpr.git
    GIT_TAG 1.12.0
)
FetchContent_MakeAvailable(cpr) # pip install meson and ninja to build

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
                tests/TestServer2D.cpp
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

target_link_libraries(drone_sim PRIVATE ImGuiLib "${GLEW_LIB_DIR}/glew32.lib" glm opengl32 gdi32 user32 shell32 advapi32 cpr::cpr nlohmann_json::nlohmann_json)

# ------------------------
# Docker ML service
# ------------------------

# Build ML image only if missing - must nuke build directory to trigger docker build again
add_custom_command(
    OUTPUT ${CMAKE_BINARY_DIR}/ml_image_built.stamp
    COMMAND docker image inspect drone_ml:latest > /dev/null 2>&1
        || docker build -t drone_ml ${CMAKE_SOURCE_DIR}/ml_service
    COMMAND ${CMAKE_COMMAND} -E touch ${CMAKE_BINARY_DIR}/ml_image_built.stamp
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/ml_service
    COMMENT "Checking/Building ML Docker image"
)

add_custom_target(ml-build
    DEPENDS ${CMAKE_BINARY_DIR}/ml_image_built.stamp
)

# Run ML container (detached so sim can run at same time)
add_custom_target(ml-run
    COMMAND docker run -d --rm -p 5000:5000 --name drone_ml_container -v ${CMAKE_SOURCE_DIR}/ml_service/ml_server:/app drone_ml
    # COMMAND docker logs -f drone_ml_container - this blocks! run in separate terminal
    DEPENDS ml-build
    COMMENT "Running ML Docker container in detached mode"
)

# Stop ML container (cleanup)
add_custom_target(ml-stop
    COMMAND docker stop drone_ml_container || true
    COMMENT "Stopping ML Docker container"
)

# ------------------------
# Super target: Run All
# ------------------------

# Ensure sim is built, ML container is up, then run sim
add_custom_target(run-all
    COMMAND $<TARGET_FILE:drone_sim>
    DEPENDS drone_sim ml-run
    COMMENT "Running simulation + ML service together"
)

# Cleanup target: stop ML container when you're done
add_custom_target(clean-all
    DEPENDS ml-stop
    COMMENT "Clean up ML service and build artifacts"
)