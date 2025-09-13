#pragma once

#include <SDL3/SDL.h>

#include "../Math/FixedTypes.h"
//#include "../Physics/Physics.h"
//#include "../Networking/Client/ClientHandler.h"

#include "GameSettings.h"
//#include "RollbackManager.h"
//#include "CacheManager.h"
//#include "Worlds/PhysicsWorld.h"

// #include "Input/InputData.h"
// #include "Input/InputManager.h"
// #include "Input/Input.h"

#include <iostream>
#include <sstream>
#include <thread>

class Game
{
public:
    explicit Game() //: worldManager(WorldManager()), playerInput(Input(playerInputKeys)), otherInput(Input(otherInputKeys)), clientHandler(ClientHandler("localhost", "50000"))
    {
        if (Initialize() != 0) return;

        //InputManager::SetupCallbacks(m_Window);

        //AddObjects();
    }

    int Initialize()
    {
        if (SDL_Init(SDL_INIT_VIDEO) != 0)
        {
            SDL_Log("SDL3 error: %s", SDL_GetError());
            return -1;
        }

        //Create and initialize a new window
        if (!SDL_CreateWindowAndRenderer("Rewind2D", 640, 480, SDL_WINDOW_RESIZABLE, &m_Window, &m_Renderer))
        {
            SDL_Log("Could not create window / renderer: %s", SDL_GetError());
            SDL_Quit();
            return -1;
        }

        m_Window = SDL_CreateWindow("Rewind2D", s_ScreenWidth, s_ScreenHeight, SDL_WINDOW_RESIZABLE);
        if (!m_Window)
        {
            SDL_Log("Could not initialize SDL %s", SDL_GetError());
            SDL_Quit();
            return -1;
        }

        return 0;
    }

    int GameLoop()
    {
        isPaused = false;
        Fixed16_16 fixedDelta = Fixed16_16(1) / Fixed16_16(SimulationFPS);
        double lastTime = static_cast<double>(SDL_GetTicks());
        Fixed16_16 accumulator(0);
        double lastTitleUpdateTime = 0.0;

        //Time tracking variables
        double updateRenderTime = 0.0;
        double sleepTime = 0.0;
        double averageFrameTime = 0.0;

        uint32_t frameCount = 0;

        bool running = true;
        SDL_Event event;

        //Start client thread
        //clientHandler.Start();

        while (running)
        {
            //Calculate delta time
            double currentTime = static_cast<double>(SDL_GetTicks());
            double frameTime = currentTime - lastTime;
            lastTime = currentTime;

            //Limit the frame time to avoid spiral of death (large lag spikes)
            frameTime = std::min(frameTime, 0.25);
            accumulator += Fixed16_16::FromFloat(frameTime);

            //Reset time tracking variables for this frame
            double frameStartTime = currentTime;
            double frameUpdateRenderStart = frameStartTime;

            if (currentTime - lastTitleUpdateTime >= 1.0f)
            {
                double totalFrameTime = updateRenderTime + sleepTime;
                double updateRenderPercent = (updateRenderTime / totalFrameTime) * 100.0;
                double sleepPercent = (sleepTime / totalFrameTime) * 100.0;

                // std::ostringstream title;
                // title << "ECS Test - FPS: " << static_cast<int>(1.0 / frameTime)
                //       << " | Update/Render: " << static_cast<int>(updateRenderPercent) << "%"
                //       << " | Waiting: " << static_cast<int>(sleepPercent) << "%";
                // glfwSetWindowTitle(m_Window, title.str().c_str());

                // Reset tracking for the next second
                updateRenderTime = 0.0;
                sleepTime = 0.0;
                lastTitleUpdateTime = currentTime;
            }

            // if (otherInput.GetKeyDown(GLFW_KEY_SPACE))
            // {
            //     isPaused = !isPaused;
            // }
            //
            // if (isPaused && otherInput.GetKeyDown(GLFW_KEY_RIGHT))
            // {
            //     Update(m_Window, fixedDelta);
            // }
            //
            // if (otherInput.GetKeyDown(GLFW_KEY_ESCAPE))
            // {
            //     glfwSetWindowShouldClose(window, true);
            //     break;
            // }
            //
            // if (otherInput.GetKeyDown(GLFW_KEY_R))
            // {
            //     isPaused = true;
            //     worldManager.Restore();
            // }
            //
            // if (otherInput.GetKey(GLFW_KEY_LEFT_CONTROL) && otherInput.GetKeyDown(GLFW_KEY_C))
            // {
            //     //Serialize current game state
            //     SDL_Log("Serializing Game State");
            //     Serialize();
            // }
            //
            // if (hasSerializedStream && otherInput.GetKey(GLFW_KEY_LEFT_CONTROL) && otherInput.GetKeyDown(GLFW_KEY_V))
            // {
            //     //Deserialize the saved game state amd override the current game state
            //     SDL_Log("Deserializing Game State");
            //     Deserialize();
            // }

            while (accumulator >= fixedDelta)
            {
                if (!isPaused)
                {
                    //Update(m_Window, fixedDelta);
                }
                accumulator -= fixedDelta;
            }

            //Render frame
            Render();

            //Handle input
            //playerInput.Update();
            //otherInput.Update();

            while (SDL_PollEvent(&event))
            {
                switch (event.type)
                {
                    case SDL_EVENT_QUIT:
                        running = false;
                        break;
                    default:
                        break;
                }
            }

            //Time spent on updates and rendering
            double frameUpdateRenderEnd = static_cast<double>(SDL_GetTicks()); //todo is actuall ms in int
            updateRenderTime += frameUpdateRenderEnd - frameUpdateRenderStart;

            //Sleep to maintain frame pacing
            double frameEndTime = static_cast<double>(SDL_GetTicks());
            double targetFrameTime = fixedDelta.ToFloating<float>();
            double elapsedFrameTime = frameEndTime - frameStartTime;

            ++frameCount;
            averageFrameTime += elapsedFrameTime;

            if (elapsedFrameTime < targetFrameTime)
            {
                double sleepDuration = targetFrameTime - elapsedFrameTime;
                std::this_thread::sleep_for(std::chrono::duration<double>(sleepDuration));
                sleepTime += sleepDuration;
            }

            //if (worldManager.GetPhysicsWorld().GetCurrentFrame() >= 1000) break;
        }

        SDL_Log("Average frame time (Lower is better) ms: ", 1000 * averageFrameTime / frameCount);

        //Stop SDL
        SDL_DestroyWindow(m_Window);
        SDL_Quit();

        return 0;
    }

    // void AddObjects()
    // {
    //     //Add objects
    //     worldManager.GetPhysicsWorld().AddObjects();
    //
    //     //Setup cache
    //     worldManager.GetPhysicsWorld().InitializeCache(&cacheManager);
    // }

    // void Update(SDL_Window* window, Fixed16_16 deltaTime)
    // {
    //     PhysicsWorld& basePhysicsWorld = worldManager.GetPhysicsWorld();
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
    //         worldManager.Reset(); //TODO: With delay frames might still be behind
    //         lastConfirmedFrame = currentFrame;
    //     }
    //     else if (lastConfirmedFrame < currentFrame)
    //     {
    //         if (currentFrame - lastConfirmedFrame >= MaxRollBackFrames)
    //         {
    //             SDL_Log("Could not rollback ", currentFrame - lastConfirmedFrame, "frames");
    //         }
    //         else
    //         {
    //             FrameNumber rollbackCount = worldManager.Restore();
    //
    //             if (rollbackCount < 0)
    //             {
    //                 SDL_Log("Could not rollback from frame ", currentFrame, "to frame ", lastConfirmedFrame);
    //             }
    //             else if (rollbackCount > 0)
    //             {
    //                 //ToDo: will always rollback when last confirmed frame is < current frame. Add checking if the received input is equal to the predicted
    //                 //SDL_Log << "Rollback ", rollbackCount, " frames");
    //
    //                 for(int32_t i = 0; i < rollbackCount; ++i)
    //                 {
    //                     worldManager.NextFrame(lastConfirmedFrame);
    //                     std::vector<Input*> inputs = clientHandler.GetAllClientInputs(basePhysicsWorld.GetCurrentFrame());
    //                     basePhysicsWorld.Update(deltaTime, inputs);
    //                 }
    //             }
    //         }
    //     }
    //
    //     worldManager.NextFrame(lastConfirmedFrame);
    //     std::vector<Input*> inputs = clientHandler.GetAllClientInputs(currentFrame);
    //     basePhysicsWorld.Update(deltaTime, inputs);
    //     clientHandler.SendGameData(basePhysicsWorld);
    // }

    void Render()
    {
        SDL_SetRenderDrawColorFloat(m_Renderer, 0, 0, 0, SDL_ALPHA_OPAQUE_FLOAT);
        SDL_RenderClear(m_Renderer);
        //worldManager.GetPhysicsWorld().Render();
        SDL_RenderPresent(m_Renderer);
    }

    // void Serialize()
    // {
    //     serializedStream.Clear();
    //     hasSerializedStream = true;
    //      worldManager.GetPhysicsWorld().Serialize(serializedStream);
    // }
    //
    // void Deserialize()
    // {
    //     Stream temporaryStream = Stream(serializedStream);
    //     worldManager.GetPhysicsWorld().Deserialize(temporaryStream);
    //     worldManager.Reset();
    // }

    //TODO: input, resize event, dsiable vsync

private:
    //WorldManager worldManager;
    //CacheManager cacheManager;

    SDL_Window* m_Window;
    SDL_Renderer* m_Renderer;

    //Input playerInput;
    //Input otherInput;

    //ClientHandler clientHandler;

    //Debug
    bool hasSerializedStream;
    //Stream serializedStream;

    bool isPaused;
};