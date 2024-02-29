#include "vk_remote.h"
#include <SDL.h>
#include <stb_image.h>
#include "SDL_image.h"
#include <chrono>
#include <algorithm>

void RemoteEngine::init()
{
    SDL_Init(SDL_INIT_VIDEO);
    IMG_Init(IMG_INIT_JPG);

    _window = SDL_CreateWindow("REMOTE", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 1700, 900, SDL_WindowFlags());

    _renderer = SDL_CreateRenderer(_window, -1, SDL_RENDERER_ACCELERATED);

    // start the image recieving thread
    _image_recv_thread = std::thread(&RemoteEngine::handle_remote, this);
}

void RemoteEngine::handle_remote()
{
    while (true)
    {
        _client.start();
    }
}

void RemoteEngine::run()
{
    bool quit = false;
    SDL_Event e;

    SDL_Texture* texture = SDL_CreateTexture(_renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, 1700, 900);


    while (!quit)
    {

        while (SDL_PollEvent(&e) != 0) {
            // close the window when user alt-f4s or clicks the X button
            if (e.type == SDL_QUIT)
                quit = true;
        }

        

        if (_client.lastImage.size() != 0)
        {
            //SDL_RWops* rw = SDL_RWFromConstMem(_client.lastImage.data(), _client.lastImage.size());
            //SDL_Texture* tex = IMG_LoadTextureTyped_RW(_renderer, rw, 0, "jpg");

            unsigned char* lockedPixels = nullptr;
            int pitch = 0;
            SDL_LockTexture(texture, nullptr, reinterpret_cast<void**>(&lockedPixels), &pitch);
            std::copy_n(_client.lastImage.data(), _client.lastImage.size(), lockedPixels);
            SDL_UnlockTexture(texture);
            

            SDL_RenderClear(_renderer);
            SDL_RenderCopy(_renderer, texture, nullptr, nullptr);
            SDL_RenderPresent(_renderer);

            //SDL_DestroyTexture(tex);

        }
    }

    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(_renderer);
    IMG_Quit();
    SDL_Quit();

    _image_recv_thread.join();

}