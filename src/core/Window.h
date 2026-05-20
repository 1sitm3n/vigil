#pragma once

#include <SDL3/SDL.h>
#include <vulkan/vulkan.h>

#include <string>
#include <vector>

namespace vigil {

class Window {
public:
    Window(int width, int height, const std::string& title);
    ~Window();

    Window(const Window&) = delete;
    Window& operator=(const Window&) = delete;
    Window(Window&&) = delete;
    Window& operator=(Window&&) = delete;

    void poll_events();
    bool should_close() const { return should_close_; }

    int width()  const { return width_; }
    int height() const { return height_; }

    // Vulkan integration
    std::vector<const char*> required_vulkan_extensions() const;
    VkSurfaceKHR create_vulkan_surface(VkInstance instance) const;

    SDL_Window* native_handle() const { return window_; }

private:
    SDL_Window* window_ = nullptr;
    int  width_         = 0;
    int  height_        = 0;
    bool should_close_  = false;
};

}  // namespace vigil
