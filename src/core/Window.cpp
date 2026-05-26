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

    SDL_SetWindowRelativeMouseMode(window_, true);
}

Window::~Window() {
    if (window_) SDL_DestroyWindow(window_);
    SDL_Quit();
}

void Window::poll_events(const EventCallback& on_event) {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        if (on_event) on_event(event);

        switch (event.type) {
            case SDL_EVENT_QUIT:
                should_close_ = true;
                break;

            case SDL_EVENT_KEY_DOWN:
                // SDL3 fires KEY_DOWN once on press then again at OS repeat
                // rate with .repeat = true. Edge triggers reject repeats so
                // Space doesn't queue rolls and N doesn't queue fake-hits.
                if (event.key.key == SDLK_ESCAPE) {
                    should_close_ = true;
                } else if (event.key.key == SDLK_SPACE && !event.key.repeat) {
                    pending_space_press_ = true;
                } else if (event.key.key == SDLK_N && !event.key.repeat) {
                    pending_n_press_ = true;
                } else if (event.key.key == SDLK_Q && !event.key.repeat) {
                    pending_q_press_ = true;
                }
                break;

            case SDL_EVENT_WINDOW_RESIZED:
                width_  = event.window.data1;
                height_ = event.window.data2;
                break;

            case SDL_EVENT_WINDOW_FOCUS_LOST:
                SDL_SetWindowRelativeMouseMode(window_, false);
                break;

            case SDL_EVENT_WINDOW_FOCUS_GAINED:
                SDL_SetWindowRelativeMouseMode(window_, true);
                break;

            case SDL_EVENT_MOUSE_BUTTON_DOWN:
                // BUTTON_DOWN fires exactly once per press (not per held
                // frame), so setting both held + pressed here works for
                // edge detection without separate prev-state tracking.
                if (event.button.button == SDL_BUTTON_RIGHT) {
                    rmb_down_           = true;
                    pending_rmb_press_  = true;   // Day 14: parry edge
                } else if (event.button.button == SDL_BUTTON_LEFT) {
                    pending_lmb_press_ = true;
                }
                break;

            case SDL_EVENT_MOUSE_BUTTON_UP:
                if (event.button.button == SDL_BUTTON_RIGHT) {
                    rmb_down_ = false;
                }
                break;

            case SDL_EVENT_MOUSE_MOTION:
                pending_mouse_dx_ += event.motion.xrel;
                pending_mouse_dy_ += event.motion.yrel;
                break;

            case SDL_EVENT_MOUSE_WHEEL:
                pending_scroll_y_ += event.wheel.y;
                break;

            default:
                break;
        }
    }
}

InputFrame Window::consume_input() {
    InputFrame frame;
    frame.mouse_dx      = pending_mouse_dx_;
    frame.mouse_dy      = pending_mouse_dy_;
    frame.scroll_y      = pending_scroll_y_;
    frame.rmb_held      = rmb_down_;
    frame.rmb_pressed   = pending_rmb_press_;
    frame.lmb_pressed   = pending_lmb_press_;
    frame.space_pressed = pending_space_press_;
    frame.n_pressed     = pending_n_press_;
    frame.q_pressed     = pending_q_press_;

    pending_mouse_dx_    = 0.0f;
    pending_mouse_dy_    = 0.0f;
    pending_scroll_y_    = 0.0f;
    pending_lmb_press_   = false;
    pending_rmb_press_   = false;
    pending_space_press_ = false;
    pending_n_press_     = false;
    pending_q_press_     = false;

    const bool* keys = SDL_GetKeyboardState(nullptr);
    frame.w_held     = keys[SDL_SCANCODE_W];
    frame.a_held     = keys[SDL_SCANCODE_A];
    frame.s_held     = keys[SDL_SCANCODE_S];
    frame.d_held     = keys[SDL_SCANCODE_D];
    frame.shift_held = keys[SDL_SCANCODE_LSHIFT] || keys[SDL_SCANCODE_RSHIFT];

    return frame;
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
