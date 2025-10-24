#include "Game/GameSettings.h"
#include "Game/Application.h"

#define SDL_MAIN_USE_CALLBACKS 1
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

Application application;
SDL_Window* window = nullptr;
SDL_Renderer* renderer = nullptr;

SDL_AppResult SDL_AppInit(void** appstate, int argc, char* argv[])
{
    SDL_SetAppMetadata("Rewind2D", "1.0", "com.SmallNightLight.Rewind2D");

    if (!SDL_Init(SDL_INIT_VIDEO))
    {
        SDL_Log("Could not initialize SDL: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    if (!SDL_CreateWindowAndRenderer("Rewind2D", s_ScreenWidth, s_ScreenHeight, SDL_WINDOW_RESIZABLE, &window, &renderer))
    {
        SDL_Log("Could not create window/renderer: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    //Control ticking custom
    SDL_SetRenderVSync(renderer, 0);

    application.Initialize(window, renderer);

    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppEvent(void* appstate, SDL_Event* event)
{
    if (event->type == SDL_EVENT_QUIT)
    {
        return SDL_APP_SUCCESS;
    }

    if (event->key.scancode == SDL_SCANCODE_ESCAPE)
    {
        return SDL_APP_SUCCESS;
    }

    application.InputCallback(event);
    //SDL_Log("KEYDOWN: scancode=%d, type: %i, repeat=%d", event->key.scancode, event->type, event->key.repeat);
    return SDL_APP_CONTINUE;
}

/* This function runs once per frame, and is the heart of the program. */
SDL_AppResult SDL_AppIterate(void* appstate)
{
    application.Update();
    return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void *appstate, SDL_AppResult result)
{

}