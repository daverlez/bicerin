#include <iostream>
#include <SDL2/SDL.h>
#include "core/system.h"

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <rom_path.gb>\n";
        return -1;
    }

    System gb_system(argv[1]);

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0) {
        std::cerr << "Error initializing SDL: " << SDL_GetError() << "\n";
        return -1;
    }

    SDL_AudioSpec desired_spec;
    SDL_zero(desired_spec);
    desired_spec.freq = 44100;
    desired_spec.format = AUDIO_F32SYS;
    desired_spec.channels = 2;
    desired_spec.samples = 2048;
    desired_spec.callback = nullptr;

    SDL_AudioDeviceID audio_device = SDL_OpenAudioDevice(nullptr, 0, &desired_spec, nullptr, 0);
    if (audio_device == 0) {
        std::cerr << "Error opening audio device: " << SDL_GetError() << "\n";
        return -1;
    }

    SDL_PauseAudioDevice(audio_device, 0);

    const int SCALE = 4;
    SDL_Window* window = SDL_CreateWindow(
        "Bicerin",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        160 * SCALE, 144 * SCALE,
        SDL_WINDOW_SHOWN
    );

    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);

    SDL_Texture* texture = SDL_CreateTexture(
        renderer,
        SDL_PIXELFORMAT_ARGB8888,
        SDL_TEXTUREACCESS_STREAMING,
        160, 144
    );

    const int TARGET_FPS = 60;
    const int FRAME_DELAY = 1000 / TARGET_FPS;

    const std::array<uint32_t, 4> THEME_GREEN = { 0xFFE0F8D0, 0xFF88C070, 0xFF346856, 0xFF081820 };
    const std::array<uint32_t, 4> THEME_GRAY  = { 0xFFFFFFFF, 0xFFAAAAAA, 0xFF555555, 0xFF000000 };

    bool running = true;
    SDL_Event event;

    while (running) {
        uint32_t frame_start = SDL_GetTicks();

        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = false;
            } else if (event.type == SDL_KEYDOWN || event.type == SDL_KEYUP) {
                bool is_pressed = (event.type == SDL_KEYDOWN);

                Joypad::Button btn;
                bool valid_key = true;

                switch (event.key.keysym.sym) {
                    case SDLK_UP:        btn = Joypad::Button::Up; break;
                    case SDLK_DOWN:      btn = Joypad::Button::Down; break;
                    case SDLK_LEFT:      btn = Joypad::Button::Left; break;
                    case SDLK_RIGHT:     btn = Joypad::Button::Right; break;
                    case SDLK_z:         btn = Joypad::Button::A; break;
                    case SDLK_x:         btn = Joypad::Button::B; break;
                    case SDLK_RETURN:    btn = Joypad::Button::Start; break;
                    case SDLK_BACKSPACE: btn = Joypad::Button::Select; break;
                    default: valid_key = false; break;
                }

                if (valid_key) {
                    if (is_pressed) gb_system.press_button(btn);
                    else gb_system.release_button(btn);
                }

                if (is_pressed) {
                    if (event.key.keysym.sym == SDLK_1) gb_system.set_palette(THEME_GREEN);
                    if (event.key.keysym.sym == SDLK_2) gb_system.set_palette(THEME_GRAY);
                }
            }
        }

        gb_system.run_frame();

        const auto& frame_buffer = gb_system.get_frame_buffer();
        SDL_UpdateTexture(texture, nullptr, frame_buffer.data(), 160 * sizeof(uint32_t));
        SDL_RenderClear(renderer);
        SDL_RenderCopy(renderer, texture, nullptr, nullptr);
        SDL_RenderPresent(renderer);

        const auto& audio_buffer = gb_system.get_audio_buffer();
        if (!audio_buffer.empty()) {
            SDL_QueueAudio(audio_device, audio_buffer.data(), audio_buffer.size() * sizeof(float));
            gb_system.clear_audio_buffer();
        }

        uint32_t frame_time = SDL_GetTicks() - frame_start;
        if (FRAME_DELAY > frame_time)
            SDL_Delay(FRAME_DELAY - frame_time);
    }

    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_CloseAudioDevice(audio_device);
    SDL_Quit();

    return 0;
}