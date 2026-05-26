#pragma once

#include <SDL3/SDL.h>
#include <vulkan/vulkan.h>

#include <functional>
#include <string>
#include <vector>

namespace vigil {

struct InputFrame {
    // Camera — Day 11: relative-mouse mode is on for the session; motion is
    // accumulated unconditionally. rmb_held tracked for Block (GDD §6);
    // rmb_pressed for parry edge-detect (Day 14).
    float mouse_dx = 0.0f;
    float mouse_dy = 0.0f;
    float scroll_y = 0.0f;
    bool  rmb_held    = false;
    bool  rmb_pressed = false;   // Day 14: edge — true for one frame on press

    // Movement (held this frame).
    bool  w_held     = false;
    bool  a_held     = false;
    bool  s_held     = false;
    bool  d_held     = false;
    bool  shift_held = false;

    // Combat (edge-triggered — true for one frame on press).
    bool  lmb_pressed   = false;
    bool  space_pressed = false;
    bool  n_pressed     = false;   // Day 14: debug fake-hit trigger
    bool  q_pressed     = false;   // Day 15: Holy Bolt cast trigger
};

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
    bool  pending_rmb_press_   = false;   // Day 14
    bool  pending_space_press_ = false;
    bool  pending_n_press_     = false;   // Day 14
    bool  pending_q_press_     = false;   // Day 15
};

}  // namespace vigil
