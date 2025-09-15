#pragma once

#include "Client.h"
#include "../Shared/Packet.h"

#include "../../Math/Stream.h"
#include "../../Common/Action/ActionCollection.h"
#include "../../ECS/ECSSettings.h"
#include "../../Game/Worlds/PhysicsWorld.h"

#include <memory>
#include <atomic>
#include <cstdint>
#include <mutex>
#include <thread>
#include <vector>
#include <set>
#include <string>
#include <unordered_map>
#include <utility>
#include <limits>

class ClientHandler
{
public:
    ClientHandler(const std::string& serverIP, const std::string& serverPort) : client(serverIP, serverPort), receivedClientID(0), running(false), connected(false), sendGameData(false) { }

    ~ClientHandler()
    {
        Stop();
    }

    void Start()
    {
        running = true;
        receivedClientID = 0;
        AddClient(0, 0);

        clientThread = std::thread(&ClientHandler::RunClientThread, this);
    }

    void Stop()
    {
        running = false;
        if (clientThread.joinable())
            clientThread.join();
    }

    void Send(const Packet& packet)
    {
        client.Send(packet.Data.GetBuffer());
    }

    bool HasNewMessages()
    {
        std::lock_guard<std::mutex> lock(messageMutex);
        return !receivedMessages.empty();
    }

    std::vector<uint8_t> PopMessage()
    {
        std::lock_guard<std::mutex> lock(messageMutex);
        if (receivedMessages.empty())
            return {};

        auto message = std::move(receivedMessages.front());
        receivedMessages.erase(receivedMessages.begin());
        return message;
    }

    void ReadMessages(PhysicsWorld& physicsWorld, bool& newGameData)
    {
        while(HasNewMessages())
        {
            std::vector<uint8_t> message = PopMessage();
            Packet packet = Packet(message);

            switch (packet.Type)
            {
                case AcceptJoin:
                {
                    if (receivedClientID == 0)
                    {
                        receivedClientID = packet.ID;
                        RemoveClient(0);

                        Debug("Received Client ID: ", receivedClientID);
                        connected = true;

                        AddClient(receivedClientID, packet.Frame);

                        //Requesting game data
                        Send(Packet(receivedClientID, RequestGameDataPacket));
                    }
                    else
                    {
                        Warning("Did not yet receive client ID??");
                    }

                    break;
                }
                case NewClientPacket:
                {
                    AddClient(packet.ID, packet.Frame);
                    break;
                }
                case RequestGameDataPacket: //Mark for later serialization
                {
                    sendGameData = true;
                    break;
                }
                case GameDataPacket:  //Deserialize the game data
                {
                    physicsWorld.Deserialize(packet.Data);
                    m_ClientActions.at(receivedClientID).JumpToFrame(physicsWorld.GetCurrentFrame());
                    Debug("Deserialized the game data");
                    newGameData = true;
                    break;
                }
                case RequestActionPacket: //Resend an action packet
                {
                    Warning("RequestActionPacket not implemented");
                    break;
                }
                case ActionPacket: //Deserialize the action packet from other clients
                {
                    Action action = Action();
                    action.Deserialize(std::move(packet.Data));
                    UpdateAction(packet.ID, action);
                    break;
                }
                default:
                {
                    Warning("Received unknown packet type ", packet.Type);
                }
            }
        }

        if (!clientsWaitingToJoin.empty())
        {
            for (auto newClient : clientsWaitingToJoin)
            {
                AddClient(newClient.first, newClient.second);
            }

            clientsWaitingToJoin.clear();
        }
    }

    ///Send back the serialized game state
    void SendGameData(const PhysicsWorld& physicsWorld)
    {
        if (!sendGameData) return;

        Stream gameData = Stream();
        physicsWorld.Serialize(gameData);
        Send(Packet(receivedClientID, GameDataPacket, 0, std::move(gameData)));
        sendGameData = false;
    }

    void UpdateAction(ClientID clientID, const Action& action)
    {
        if (!m_ClientActions.contains(clientID)) return;

        m_ClientActions.at(clientID).AddAction(action);
    }

    void SendAction(Action& action)
    {
        if (receivedClientID == 0) return;

        Stream stream = Stream();
        action.Serialize(stream);
        Packet packet(receivedClientID, ActionPacket, action.Frame, std::move(stream));
        Send(packet);
    }

    void AddClient(ClientID clientID, uint32_t frame)
    {
        if (m_ClientActions.contains(clientID))
        {
            Warning("Can not add a client that already exists");
            return;
        }

        clientIDs.insert(clientID);
        m_ClientActions.emplace(clientID, ActionCollection(frame, MaxRollBackFrames * 2));

        //Add actions for the first few frames to avoid missing actions
        for(uint32_t i = frame; i < frame + 5; ++i)
        {
            Action action = Action(i);
            m_ClientActions.at(clientID).AddAction(action);
        }
    }

    void RemoveClient(ClientID clientID)
    {
        if (clientID == receivedClientID)
        {
            Warning("Cannot remove itself");
            return;
        }

        if (!m_ClientActions.contains(clientID))
        {
            Warning("Can not remove a client that does not exists");
            return;
        }

        clientIDs.erase(clientID);
        m_ClientActions.erase(clientID);
    }

    ClientID GetClientID() const
    {
        return receivedClientID;
    }

    const std::set<ClientID>& GetAllClientIDs() const
    {
        return clientIDs;
    }

    Action GetClientAction(ClientID clientID, uint32_t frame)
    {
        if (!m_ClientActions.contains(clientID))
        {
            throw std::invalid_argument("clientID not found");
        }

        ActionCollection& actionCollection= m_ClientActions.at(clientID);

        if (actionCollection.NeedsPrediction(frame))
        {
            return actionCollection.GetPredictedAction(frame);
        }

        return actionCollection.GetAction(frame);
    }

    std::vector<Action> GetAllClientActions(uint32_t frame)
    {
        std::set<ClientID> clientIDSet = GetAllClientIDs();
        std::vector<Action> result(clientIDSet.size());

        int i = 0;
        for (ClientID clientID : clientIDSet)
        {
            result[i] = GetClientAction(clientID, frame);
            ++i;
        }

        return result;
    }

    uint32_t GetRollbacks(uint32_t currentFrame) const
    {
        uint32_t lastConfirmedFrame = GetLastConfirmedFrame();

        if (lastConfirmedFrame >= currentFrame) return 0;

        return currentFrame - lastConfirmedFrame;
    }

    uint32_t GetLastConfirmedFrame() const
    {
        uint32_t lastConfirmedFrame = std::numeric_limits<uint32_t>::max();

        for (const auto& clientAction : m_ClientActions)
        {
            uint32_t lastCompletedFrame = clientAction.second.GetLastCompletedFrame();

            if (lastCompletedFrame < lastConfirmedFrame)
            {
                lastConfirmedFrame = lastCompletedFrame;
            }
        }

        return lastConfirmedFrame;
    }

private:
    void RunClientThread()
    {
        //Connect to server
        client.Send(std::vector<uint8_t> { });

        while (running)
        {
            //Wait for client ID during initialization
            if (receivedClientID == 0)
            {
                if (client.HasMessages())
                {
                    auto message = client.PopMessage();
                    Packet packet(message);

                    if (packet.Type == AcceptJoin)
                    {
                        //Add packet to queue so it can be evaluated later
                        receivedMessages.push_back(packet.Data.GetBuffer());

                    }
                    else if (packet.Type == NewClientPacket)
                    {
                        clientsWaitingToJoin[packet.ID] = packet.Frame;
                    }
                    else
                    {
                        receivedMessages.push_back(packet.Data.GetBuffer());
                    }
                }
            }
            else
            {
                //Process messages and push them to the queue
                if (client.HasMessages())
                {
                    std::vector<uint8_t> message = client.PopMessage();
                    std::lock_guard<std::mutex> lock(messageMutex);
                    receivedMessages.push_back(std::move(message));
                }
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }

private:
    Client client;
    uint32_t receivedClientID;

    std::thread clientThread;
    std::atomic<bool> running;

    std::mutex messageMutex;
    std::vector<std::vector<uint8_t>> receivedMessages;
    std::unordered_map<ClientID, uint32_t> clientsWaitingToJoin;

    std::set<ClientID> clientIDs { };
    std::unordered_map<ClientID, ActionCollection> m_ClientActions;

    bool connected;
    bool sendGameData;
};