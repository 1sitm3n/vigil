#include "core/Window.h"

#include <SDL3/SDL_vulkan.h>

#include <stdexcept>

namespace vigil {

Window::Window(int width, int height, const std::string& title)
    : width_(width), height_(height) {
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS)) {
        throw std::runtime_error(std::string{"SDL_Init failed: "} + SDL_GetError());
    }

    const SDL_WindowFlags flags = SDL_WINDOW_VULKAN
                                | SDL_WINDOW_RESIZABLE
                                | SDL_WINDOW_HIGH_PIXEL_DENSITY;

    window_ = SDL_CreateWindow(title.c_str(), width, height, flags);
    if (!window_) {
        SDL_Quit();
        throw std::runtime_error(std::string{"SDL_CreateWindow failed: "} + SDL_GetError());
    }
}

Window::~Window() {
    if (window_) SDL_DestroyWindow(window_);
    SDL_Quit();
}

void Window::poll_events() {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        switch (event.type) {
            case SDL_EVENT_QUIT:
                should_close_ = true;
                break;
            case SDL_EVENT_KEY_DOWN:
                if (event.key.key == SDLK_ESCAPE) should_close_ = true;
                break;
            case SDL_EVENT_WINDOW_RESIZED:
                width_  = event.window.data1;
                height_ = event.window.data2;
                break;
            default:
                break;
        }
    }
}

std::vector<const char*> Window::required_vulkan_extensions() const {
    Uint32 count = 0;
    const char* const* exts = SDL_Vulkan_GetInstanceExtensions(&count);
    if (!exts) {
        throw std::runtime_error(std::string{"SDL_Vulkan_GetInstanceExtensions failed: "} +
                                 SDL_GetError());
    }
    return std::vector<const char*>(exts, exts + count);
}

VkSurfaceKHR Window::create_vulkan_surface(VkInstance instance) const {
    VkSurfaceKHR surface = VK_NULL_HANDLE;
    if (!SDL_Vulkan_CreateSurface(window_, instance, nullptr, &surface)) {
        throw std::runtime_error(std::string{"SDL_Vulkan_CreateSurface failed: "} +
                                 SDL_GetError());
    }
    return surface;
}

}  // namespace vigil
