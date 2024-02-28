#include "vk_remote.h"
#include <SDL.h>
#include <stb_image.h>
#include "SDL_image.h"

void RemoteEngine::init()
{
    SDL_Init(SDL_INIT_VIDEO);
    IMG_Init(IMG_INIT_JPG);

    _window = SDL_CreateWindow("REMOTE", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 1700, 900, SDL_WindowFlags());

    _renderer = SDL_CreateRenderer(_window, -1, SDL_RENDERER_ACCELERATED);

    // start the image recieving thread
    _image_recv_thread = std::thread(&NetworkClient::start, &_client);
}

void RemoteEngine::run()
{
    bool quit = false;
    SDL_Event event;

    while (!quit)
    {
        SDL_WaitEvent(&event);

        switch (event.type)
        {
        case SDL_QUIT:
            quit = true;
            break;
        }

        

        if (_client.lastImage.size() != 0)
        {
            SDL_RWops* rw = SDL_RWFromConstMem(_client.lastImage.data(), _client.lastImage.size());
            SDL_Texture* tex = IMG_LoadTextureTyped_RW(_renderer, rw, 0, "jpg");

            SDL_RenderClear(_renderer);
            SDL_RenderCopy(_renderer, tex, nullptr, nullptr);
            SDL_RenderPresent(_renderer);

            SDL_DestroyTexture(tex);

        }

    }


    SDL_DestroyRenderer(_renderer);
    IMG_Quit();
    SDL_Quit();

    _image_recv_thread.join();

}