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
        rollbackManager.Initialize(renderer, &cacheManager);
        m_PlayerAction.Initialize(&m_ActionManager);
        m_SecondaryAction.Initialize(&m_ActionManager);
        Temp();

        m_Paused = false;
        m_LastTick = SDL_GetTicksNS();
        m_Accumulator = 0;

        AddObjects();
    }

    void Temp()
    {
        m_ActionManager.RegisterAction(SDL_SCANCODE_W);
        m_ActionManager.RegisterAction(SDL_SCANCODE_S);
        m_ActionManager.RegisterAction(SDL_SCANCODE_A);
        m_ActionManager.RegisterAction(SDL_SCANCODE_D);
        m_ActionManager.RegisterAction(SDL_SCANCODE_Q);
        m_ActionManager.RegisterAction(SDL_SCANCODE_E);
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
        constexpr Uint64 deltaNS = 1'000'000'000 / SimulationFPS;

        Uint64 currentTime = SDL_GetTicksNS();
        Uint64 frameTime = currentTime - m_LastTick;
        m_LastTick = currentTime;

        //Limit the frame time to avoid spiral of death (large lag spikes)
        frameTime = std::min(frameTime, MaxFrameTimeNS);
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
    }
int fgbewrhiuo;
    void Step()
    {
        constexpr Fixed16_16 fixedDelta = Fixed16_16(1) / Fixed16_16(SimulationFPS);
        PhysicsWorld& basePhysicsWorld = rollbackManager.GetPhysicsWorld();
        FrameNumber currentFrame = basePhysicsWorld.GetCurrentFrame();
        FrameNumber lastConfirmedFrame = fgbewrhiuo++;//clientHandler.GetLastConfirmedFrame();

        rollbackManager.NextFrame(lastConfirmedFrame);
        //std::vector<Input*> inputs = clientHandler.GetAllClientInputs(currentFrame);
        basePhysicsWorld.Update(fixedDelta);
        //clientHandler.SendGameData(basePhysicsWorld);
    }

    // void Step()
    // {
    //     constexpr Fixed16_16 fixedDelta = Fixed16_16(1) / Fixed16_16(SimulationFPS);
    //
    //     PhysicsWorld& basePhysicsWorld = rollbackManager.GetPhysicsWorld();
    //     FrameNumber currentFrame = basePhysicsWorld.GetCurrentFrame();
    //
    //     //Update the input of this client
    //     InputData input = playerInput.GetInputData(currentFrame);
    //     clientHandler.SendInput(input);
    //     clientHandler.UpdateInput(clientHandler.GetClientID(), input);
    //
    //     //Update the input of other clients and handle other packets, deserializing new game state
    //     bool newGameData = false;
    //     clientHandler.ReadMessages(basePhysicsWorld, newGameData);
    //
    //     FrameNumber lastConfirmedFrame = clientHandler.GetLastConfirmedFrame();
    //
    //     if (RollbackDebugMode)
    //     {
    //         if (currentFrame < MaxRollBackFrames)
    //             lastConfirmedFrame = 1;
    //         else
    //             lastConfirmedFrame = currentFrame - (MaxRollBackFrames - 1);
    //     }
    //
    //     if (lastConfirmedFrame > currentFrame)
    //     {
    //         //Should only be possible at start
    //         lastConfirmedFrame = currentFrame;
    //     }
    //
    //     if (newGameData)
    //     {
    //         rollbackManager.Reset(); //TODO: With delay frames might still be behind
    //         lastConfirmedFrame = currentFrame;
    //     }
    //     else if (lastConfirmedFrame < currentFrame)
    //     {
    //         if (currentFrame - lastConfirmedFrame >= MaxRollBackFrames)
    //         {
    //             std::cout << "Could not rollback " << currentFrame - lastConfirmedFrame << "frames" << std::endl;
    //         }
    //         else
    //         {
    //             int32_t rollbackCount = rollbackManager.Restore();
    //
    //             if (rollbackCount < 0)
    //             {
    //                 std::cout << "Could not rollback from frame " << currentFrame << "to frame " << lastConfirmedFrame << std::endl;
    //             }
    //             else if (rollbackCount > 0)
    //             {
    //                 //ToDo: will always rollback when last confirmed frame is < current frame. Add checking if the received input is equal to the predicted
    //                 //std::cout << "Rollback " << rollbackCount << " frames" << std::endl;
    //
    //                 for(int32_t i = 0; i < rollbackCount; ++i)
    //                 {
    //                     rollbackManager.NextFrame(lastConfirmedFrame);
    //                     std::vector<Input*> inputs = clientHandler.GetAllClientInputs(basePhysicsWorld.GetCurrentFrame());
    //                     basePhysicsWorld.Update(fixedDelta, inputs);
    //                 }
    //             }
    //         }
    //     }
    //
    //     rollbackManager.NextFrame(lastConfirmedFrame);
    //     std::vector<Input*> inputs = clientHandler.GetAllClientInputs(currentFrame);
    //     basePhysicsWorld.Update(deltaTime, inputs);
    //     clientHandler.SendGameData(basePhysicsWorld);
    // }

    void Render()
    {
        //Clear background
        SDL_SetRenderDrawColorFloat(m_Renderer, 0, 0, 0, SDL_ALPHA_OPAQUE_FLOAT);
        SDL_RenderClear(m_Renderer);

        //Render entities
        rollbackManager.GetPhysicsWorld().Render(&m_PlayerAction);

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
