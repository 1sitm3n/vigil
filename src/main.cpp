// =============================================================================
//  Vigil — Day 1
//  Foundation check: SDL3 links, Vulkan loader resolves, headers match.
//  Nothing renders yet. That starts Day 2.
// =============================================================================

#include <SDL3/SDL.h>
#include <vulkan/vulkan.h>

#include <cstdint>
#include <cstdio>

int main(int /*argc*/, char* /*argv*/[]) {
    std::printf("================ Vigil — Day 1 foundation check ================\n");

    // --- SDL3 ---
    std::printf("Compiled against SDL %d.%d.%d\n",
                SDL_MAJOR_VERSION, SDL_MINOR_VERSION, SDL_MICRO_VERSION);

    const int sdl_linked = SDL_GetVersion();
    std::printf("Linked   against SDL %d.%d.%d\n",
                SDL_VERSIONNUM_MAJOR(sdl_linked),
                SDL_VERSIONNUM_MINOR(sdl_linked),
                SDL_VERSIONNUM_MICRO(sdl_linked));

    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS)) {
        std::fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
        return 1;
    }

    // --- Vulkan ---
    std::printf("Compiled against Vulkan header %u.%u.%u (variant %u)\n",
                VK_API_VERSION_MAJOR(VK_HEADER_VERSION_COMPLETE),
                VK_API_VERSION_MINOR(VK_HEADER_VERSION_COMPLETE),
                VK_API_VERSION_PATCH(VK_HEADER_VERSION_COMPLETE),
                VK_API_VERSION_VARIANT(VK_HEADER_VERSION_COMPLETE));

    uint32_t vk_loader_version = 0;
    if (vkEnumerateInstanceVersion(&vk_loader_version) == VK_SUCCESS) {
        std::printf("Linked   against Vulkan loader %u.%u.%u (variant %u)\n",
                    VK_API_VERSION_MAJOR(vk_loader_version),
                    VK_API_VERSION_MINOR(vk_loader_version),
                    VK_API_VERSION_PATCH(vk_loader_version),
                    VK_API_VERSION_VARIANT(vk_loader_version));
    } else {
        std::fprintf(stderr, "vkEnumerateInstanceVersion failed — loader not found?\n");
        SDL_Quit();
        return 1;
    }

    uint32_t layer_count = 0;
    vkEnumerateInstanceLayerProperties(&layer_count, nullptr);
    std::printf("Vulkan instance layers available: %u\n", layer_count);

    uint32_t ext_count = 0;
    vkEnumerateInstanceExtensionProperties(nullptr, &ext_count, nullptr);
    std::printf("Vulkan instance extensions available: %u\n", ext_count);

    SDL_Quit();

    std::printf("\n[Vigil] Day 1 — foundation OK.\n");
    return 0;
}
