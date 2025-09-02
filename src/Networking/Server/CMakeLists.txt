cmake_minimum_required(VERSION 3.27)
project(Server LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 23)

if(WIN32)
    add_compile_definitions(_WIN32_WINNT=0x0A00)
endif()

add_executable(Server main.cpp
        Server.h

        ../Shared/Log.h
        ../Shared/Packet.h
        ../Shared/ThreadedQueue.h
        ../Shared/NetworkingSettings.h
)

if(TARGET SERVER)
    add_subdirectory(../../Math)
endif()

#ASIO
target_include_directories(Server PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../../extern/asio/asio/include)
find_package(Threads REQUIRED)
target_link_libraries(Server PRIVATE Threads::Threads)
target_compile_definitions(Server PRIVATE ASIO_STANDALONE)