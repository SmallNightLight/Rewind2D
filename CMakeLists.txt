cmake_minimum_required(VERSION 3.27)
project(Game LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 23)

add_executable(Game src/main.cpp
        src/Game/Game.h
        src/Game/GameSettings.h
        src/Game/Application.h
        src/Game/RollbackManager.h
        src/Game/World.h
        src/Game/Worlds/PhysicsWorld.h
#        src/Game/Input/InputData.h
#        src/Game/Input/InputManager.h
        src/Game/Input/ActionManager.h
#        src/Game/Input/Input.h
#        src/Game/Input/InputCollection.h
#        src/Game/Input/Input.cpp
        src/Game/CacheManager.h
        src/Game/Worlds/PhysicsWorldData.h
        src/Game/Input/Action.h

        #src/Rendering/Shader.h
        #src/Rendering/Camera.h
)

add_subdirectory(src/ECS)
add_subdirectory(src/Math)
add_subdirectory(src/Physics)
#add_subdirectory(src/Networking/Client)
#add_subdirectory(src/Networking/Server)

if (WIN32)
    find_package(OpenGL REQUIRED)
endif()

add_subdirectory(extern/SDL EXCLUDE_FROM_ALL)
target_link_libraries(Game PRIVATE SDL3::SDL3)

if (WIN32)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pg")
endif()