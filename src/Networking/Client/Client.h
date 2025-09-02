#pragma once

#include "../Shared/NetworkingSettings.h"
#include "../Shared/ThreadedQueue.h"
#include "../Shared/Log.h"

#include <string>
#include <array>
#include <vector>
#include <thread>
#include <asio.hpp>

class Client
{
public:
    Client(const std::string& serverIP, const std::string& serverPort) : socket(m_context, asio::ip::udp::endpoint(asio::ip::udp::v4(), 0)), service_thread(&Client::RunService, this)
    {
        asio::ip::udp::resolver resolver(m_context);
        auto results = resolver.resolve(asio::ip::udp::v4(), serverIP, serverPort);
        server_endpoint = *results.begin();
    }

    ~Client()
    {
        m_context.stop();
        service_thread.join();
    }

    void Send(const std::vector<uint8_t>& data)
    {
        socket.send_to(asio::buffer(data), server_endpoint);
    }

    bool HasMessages()
    {
        return !incomingMessages.empty();
    }

    std::vector<uint8_t> PopMessage()
    {
        if (incomingMessages.empty())
            throw std::logic_error("No messages to pop");

        return incomingMessages.pop();
    }

private:
    void Receive(const std::error_code& error, std::size_t bytes_transferred)
    {
        if (!error)
        {
            std::vector<uint8_t> message(recv_buffer.data(), recv_buffer.data() + bytes_transferred);
            incomingMessages.push(message);
        }
        else
        {
            Error("Client::Receive: ", error);
        }

        StartReceive();
    }

    void StartReceive()
    {
        socket.async_receive_from(asio::buffer(recv_buffer), remote_endpoint, [this](std::error_code ec, std::size_t bytes_recvd)
        {
            this->Receive(ec, bytes_recvd);
        });
    }

    void RunService()
    {
        StartReceive();

        while (!m_context.stopped())
        {
            try
            {
                m_context.run();
            }
            catch (const std::exception& e)
            {
                Warning("Client: network exception: ", e.what());
            }
            catch (...)
            {
                Error("Unknown exception in client network thread");
            }
        }

        Debug("Server network thread stopped");
    }

private:
    asio::io_context m_context;
    asio::ip::udp::socket socket;
    asio::ip::udp::endpoint server_endpoint;
    asio::ip::udp::endpoint remote_endpoint;
    std::array<uint8_t, NetworkBufferSize> recv_buffer { };
    std::thread service_thread;

    ThreadedQueue<std::vector<uint8_t>> incomingMessages;
};