#pragma once

#include "GameSettings.h"
#include "../Math/FixedTypes.h"
#include "../Physics/Physics.h"
#include "RollbackManager.h"
#include "../Networking/Client/ClientHandler.h"
#include "../Common/Action/Action.h"
#include "../Common/Action/ActionManager.h"
#include "CacheManager.h"

#include <SDL3/SDL.h>
#include <thread>

class Application
{
public:
    Application() : m_ClientHandler(ClientHandler("localhost", "50000")), m_Window(nullptr), m_Renderer(nullptr), m_Paused(false), m_LastTick(0), m_Accumulator(0) { }

    void Initialize(SDL_Window* window, SDL_Renderer* renderer)
    {
        m_Window = window;
        m_Renderer = renderer;

        cacheManager = CacheManager();
        rollbackManager.Initialize(renderer, &m_PlayerAction, &cacheManager);
        m_PlayerAction.Initialize(&m_ActionManager);
        m_SecondaryAction.Initialize(&m_ActionManager);
        Temp();

        m_Paused = false;
        m_LastTick = SDL_GetTicksNS();
        m_Accumulator = 0;

        m_ClientHandler.Start();

        AddObjects();
    }

    void Temp()
    {
        m_ActionManager.RegisterKeyAction(SDL_SCANCODE_W);
        m_ActionManager.RegisterKeyAction(SDL_SCANCODE_S);
        m_ActionManager.RegisterKeyAction(SDL_SCANCODE_A);
        m_ActionManager.RegisterKeyAction(SDL_SCANCODE_D);
        m_ActionManager.RegisterKeyAction(SDL_SCANCODE_Q);
        m_ActionManager.RegisterKeyAction(SDL_SCANCODE_E);
    }

    void AddObjects()
    {
        //Add objects
        rollbackManager.GetPhysicsWorld().AddObjects();
    }

    inline void InputCallback(SDL_Event* event)
    {
        m_PlayerAction.Callback(event);
    }

    void Update()
    {
        constexpr Uint64 deltaNS = 1'000'000'000 / s_SimulationFPS;

        Uint64 currentTime = SDL_GetTicksNS();
        Uint64 frameTime = currentTime - m_LastTick;
        m_LastTick = currentTime;

        //Limit the frame time to avoid spiral of death (large lag spikes)
        frameTime = std::min(frameTime, s_MaxFrameTimeNS);
        m_Accumulator += frameTime;

        while (m_Accumulator >= deltaNS)
        {
            if (!m_Paused)
            {
                Step();
            }
            m_Accumulator -= deltaNS;
        }

        //Render frame //todo add rendering fps
        Render();

        //Sleep to maintain frame pacing
        Uint64 frameEndTime = SDL_GetTicksNS();
        Uint64 elapsedFrameTime = frameEndTime - m_LastTick;

        if (elapsedFrameTime < deltaNS)
        {
            Uint64 sleepDuration = deltaNS - elapsedFrameTime;
            std::this_thread::sleep_for(std::chrono::nanoseconds(sleepDuration));
        }

        static Uint64 lastTitleUpdate = SDL_GetTicksNS();
        Uint64 now = SDL_GetTicksNS();

        if (now - lastTitleUpdate >= 500'000'000)
        {
            char title[128];
            snprintf(title, sizeof(title), "Rewind2D - Frame: %.3f ms", static_cast<double>(elapsedFrameTime) / 1'000'000.0);
            SDL_SetWindowTitle(m_Window, title);

            lastTitleUpdate = now;
        }
    }

    void Step()
    {
        constexpr Fixed16_16 fixedDelta = Fixed16_16(1) / Fixed16_16(s_SimulationFPS);

        PhysicsWorld& basePhysicsWorld = rollbackManager.GetPhysicsWorld();
        FrameNumber currentFrame = basePhysicsWorld.GetCurrentFrame();

        //Update the input of this client
        m_PlayerAction.Frame = currentFrame;

        m_ClientHandler.SendAction(m_PlayerAction);
        m_ClientHandler.UpdateAction(m_ClientHandler.GetClientID(), m_PlayerAction);

        //Update the input of other clients and handle other packets, deserializing new game state
        bool newGameData = false;
        m_ClientHandler.ReadMessages(basePhysicsWorld, newGameData);

        FrameNumber lastConfirmedFrame = m_ClientHandler.GetLastConfirmedFrame();

        if (s_RollbackDebugMode)
        {
            if (currentFrame < s_MaxRollBackFrames)
                lastConfirmedFrame = 1;
            else
                lastConfirmedFrame = currentFrame - (s_MaxRollBackFrames - 1);
        }

        if (lastConfirmedFrame > currentFrame)
        {
            //Should only be possible at start
            lastConfirmedFrame = currentFrame;
        }

        if (newGameData)
        {
            rollbackManager.Reset(); //TODO: With delay frames might still be behind
            lastConfirmedFrame = currentFrame;
        }
        else if (lastConfirmedFrame < currentFrame)
        {
            if (currentFrame - lastConfirmedFrame >= s_MaxRollBackFrames)
            {
                std::cout << "Could not rollback " << currentFrame - lastConfirmedFrame << "frames" << std::endl;
            }
            else
            {
                FrameNumber restoredFrame = rollbackManager.Restore();

                if (lastConfirmedFrame < restoredFrame)
                {
                    std::cout << "Could not rollback from frame " << currentFrame << " to frame " << lastConfirmedFrame << std::endl;
                }
                else
                {
                    FrameNumber rollbackCount = currentFrame - restoredFrame;

                    //ToDo: will always rollback when last confirmed frame is < current frame. Add checking if the received input is equal to the predicted

                    for(FrameNumber i = 0; i < rollbackCount; ++i)
                    {
                        rollbackManager.NextFrame(lastConfirmedFrame);
                        std::vector<Action> actions = m_ClientHandler.GetAllClientActions(basePhysicsWorld.GetCurrentFrame());
                        basePhysicsWorld.Update(fixedDelta, actions);
                    }
                }
            }
        }

        rollbackManager.NextFrame(lastConfirmedFrame);
        std::vector<Action> actions = m_ClientHandler.GetAllClientActions(currentFrame);
        basePhysicsWorld.Update(fixedDelta, actions);
        m_ClientHandler.SendGameData(basePhysicsWorld);
        m_PlayerAction.Update();
    }

    void Render()
    {
        //Clear background
        SDL_SetRenderDrawColorFloat(m_Renderer, 0, 0, 0, SDL_ALPHA_OPAQUE_FLOAT);
        SDL_RenderClear(m_Renderer);

        //Render entities
        rollbackManager.GetPhysicsWorld().Render();

        //Switch render frame
        SDL_RenderPresent(m_Renderer);
    }

private:
    ClientHandler m_ClientHandler;

    SDL_Window* m_Window;
    SDL_Renderer* m_Renderer;

    RollbackManager rollbackManager;
    CacheManager cacheManager;
    ActionManager m_ActionManager;
    Action m_PlayerAction;
    Action m_SecondaryAction;

    bool m_Paused;
    Uint64 m_LastTick;
    Uint64 m_Accumulator;
};