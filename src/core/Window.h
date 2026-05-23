#pragma once

#include <SDL3/SDL.h>
#include <vulkan/vulkan.h>

#include <functional>
#include <string>
#include <vector>

namespace vigil {

struct InputFrame {
    // Camera
    float mouse_dx = 0.0f;         // accumulated relative motion while RMB held
    float mouse_dy = 0.0f;
    float scroll_y = 0.0f;         // accumulated wheel ticks
    bool  rmb_held = false;

    // Movement (held this frame)
    bool  w_held     = false;
    bool  a_held     = false;
    bool  s_held     = false;
    bool  d_held     = false;
    bool  shift_held = false;

    // Combat (edge-triggered — true for one frame on press)
    bool  lmb_pressed   = false;
    bool  space_pressed = false;
};

// Day-10 refactor: poll_events takes an optional per-event callback so the
// ImGui SDL3 backend can see raw SDL_Event payloads BEFORE Window's own
// switch consumes them. ImGui needs every event (keyboard, mouse, text) to
// keep its IO state correct, so the callback fires first in the loop.
using EventCallback = std::function<void(const SDL_Event&)>;

class Window {
public:
    Window(int width, int height, const std::string& title);
    ~Window();

    Window(const Window&) = delete;
    Window& operator=(const Window&) = delete;
    Window(Window&&) = delete;
    Window& operator=(Window&&) = delete;

    void       poll_events(const EventCallback& on_event = {});
    InputFrame consume_input();
    bool       should_close() const { return should_close_; }

    int width()  const { return width_; }
    int height() const { return height_; }

    std::vector<const char*> required_vulkan_extensions() const;
    VkSurfaceKHR create_vulkan_surface(VkInstance instance) const;

    SDL_Window* native_handle() const { return window_; }

private:
    SDL_Window* window_ = nullptr;
    int  width_         = 0;
    int  height_        = 0;
    bool should_close_  = false;

    float pending_mouse_dx_    = 0.0f;
    float pending_mouse_dy_    = 0.0f;
    float pending_scroll_y_    = 0.0f;
    bool  rmb_down_            = false;
    bool  pending_lmb_press_   = false;
    bool  pending_space_press_ = false;
};

}  // namespace vigil
