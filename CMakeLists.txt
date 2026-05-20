cmake_minimum_required(VERSION 3.25)

project(vigil
    VERSION 0.1.0
    DESCRIPTION "A single-arena dark-fantasy combat gauntlet"
    LANGUAGES CXX
)

# === C++ standard ===
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Default to Release if user didn't specify
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "RelWithDebInfo" "MinSizeRel")
endif()

# Export compile_commands.json for clangd / IntelliSense
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Put the binary at build/vigil
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")

message(STATUS "Vigil ${PROJECT_VERSION} — ${CMAKE_BUILD_TYPE} build")
message(STATUS "Compiler: ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "System:   ${CMAKE_SYSTEM_NAME} ${CMAKE_SYSTEM_PROCESSOR}")

# === Dependencies ===

include(FetchContent)
set(FETCHCONTENT_QUIET FALSE)

# --- Vulkan SDK ---
find_package(Vulkan REQUIRED)
message(STATUS "Vulkan SDK version: ${Vulkan_VERSION}")
message(STATUS "Vulkan include dir: ${Vulkan_INCLUDE_DIRS}")
message(STATUS "Vulkan library:     ${Vulkan_LIBRARIES}")

# --- SDL3 ---
# Bump GIT_TAG to the latest release tag from
#   https://github.com/libsdl-org/SDL/releases  (use a release-3.x.y tag)
FetchContent_Declare(SDL3
    GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
    GIT_TAG        release-3.2.0
    GIT_SHALLOW    TRUE
)
set(SDL_SHARED        OFF CACHE BOOL "" FORCE)
set(SDL_STATIC        ON  CACHE BOOL "" FORCE)
set(SDL_TEST_LIBRARY  OFF CACHE BOOL "" FORCE)
set(SDL_TESTS         OFF CACHE BOOL "" FORCE)
set(SDL_EXAMPLES      OFF CACHE BOOL "" FORCE)
set(SDL_INSTALL_TESTS OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(SDL3)

# --- glm ---
FetchContent_Declare(glm
    GIT_REPOSITORY https://github.com/g-truc/glm.git
    GIT_TAG        1.0.1
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(glm)

# === Executable ===

add_executable(vigil
    src/main.cpp
)

target_link_libraries(vigil PRIVATE
    Vulkan::Vulkan
    SDL3::SDL3-static
    glm::glm
)

target_include_directories(vigil PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/third_party
)

if(APPLE)
    target_compile_definitions(vigil PRIVATE
        VK_USE_PLATFORM_METAL_EXT
    )
endif()

# Warnings baseline
if(MSVC)
    target_compile_options(vigil PRIVATE /W4 /permissive-)
else()
    target_compile_options(vigil PRIVATE
        -Wall -Wextra -Wpedantic
        -Wno-unused-parameter
        -Wno-missing-field-initializers
    )
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Debug" AND NOT MSVC)
    target_compile_options(vigil PRIVATE -g -O0)
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Release" AND NOT MSVC)
    target_compile_options(vigil PRIVATE -O3)
endif()
