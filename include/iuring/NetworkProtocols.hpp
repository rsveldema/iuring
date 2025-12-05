#pragma once

#include <cassert>
#include <chrono>
#include <cstdint>
#include <stdlib.h>

namespace iuring
{

enum class SocketKind
{
    MULTICAST_PACKET_SOCKET,
    SERVER_STREAM_SOCKET,
    UNICAST_CLIENT_SOCKET
};

enum class SocketType
{
    UNKNOWN,

    IPV4_UDP,
    IPV4_TCP,

    IPV6_UDP,
    IPV6_TCP
};

enum class SocketPortID : u_int16_t
{
    UNENCRYPTED_WEB_PORT = 80,

    PTP_PORT_EVENT = 319,
    PTP_PORT_GENERAL = 320,

    ENCRYPTED_WEB_PORT = 443,

    LAST_PRIVILEDGED_PORT_ID = 1024,

    LOCAL_WEB_PORT = 8080,

    // Session Announcement Protocol
    SAP_PORT_EVENT = 9875,

    // rtp audio bcast
    RTP_AUDIO_PORT = 5004,

    MDNS_PORT=5353,

    UNKNOWN = 0xffff,
};

enum class timetolive_t : uint8_t
{
    PTP_TTL = 16,
    RTP_TTL = 32,
    NORMAL_TTL = 58,
    MDNS_TTL = 255
};

enum class dscp_t : uint8_t
{
    CS0 = 0,
    CS1 = 8,
    CS2 = 16,
    CS3 = 24,
    CS4 = 32,
    CS5 = 40,
    CS6 = 48,
    CS7 = 56,

    AF11 = 10,
    AF12 = 12,
    AF13 = 14,

    AF21 = 18,
    AF22 = 20,
    AF23 = 22,

    AF31 = 26,
    AF32 = 28,
    AF33 = 30,

    AF41 = 34,
    AF42 = 36,
    AF43 = 38,

    VOICE_ADMIT = 44,
    EXPEDITED_FORWARDING = 46,

    BEST_EFFORT = CS0,

    // RAVENNA and Dante use other DSCP defaults (CS6 (48) for PTP, EF (46) for
    // audio),

    AES67_PTP_EVENT = EXPEDITED_FORWARDING,
    AES67_PTP_GENERAL = BEST_EFFORT,
    AES67_RTP = AF41,

    RAV_DANTE_PTP_EVENT = CS6,
    RAV_DANTE_PTP_GENERAL = BEST_EFFORT,
    RAV_DANTE_RTP = EXPEDITED_FORWARDING
};
} // namespace iuring




template <>
struct std::formatter<iuring::SocketPortID> {
    constexpr auto parse(std::format_parse_context& ctx) {
        return ctx.begin();
    }

    auto format(iuring::SocketPortID c, std::format_context& ctx) const {
        return std::format_to(ctx.out(), "{}", static_cast<uint16_t>(c));
    }
};

