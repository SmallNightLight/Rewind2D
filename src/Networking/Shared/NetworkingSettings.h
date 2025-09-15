#pragma once

#include <cstdint>
#include <asio.hpp>

static constexpr int NetworkBufferSize = 16384;
static constexpr uint32_t InputPacketDelay = 50; //Adding a delay in milliseconds to test rollbacks

typedef uint32_t ClientID;
typedef asio::ip::udp::endpoint ClientEndpoint;

enum PacketType
{
    RequestJoinPacket, //Also with empty message
    AcceptJoin,
    NewClientPacket,
    RequestGameDataPacket,
    GameDataPacket,
    RequestActionPacket, //For resending input packet, input send by default
    ActionPacket
};