#pragma once

namespace iuring
{
enum class SocketPortID : u_int16_t
{
    UNENCRYPTED_WEB_PORT = 80,

    PTP_PORT_EVENT = 319,
    PTP_PORT_GENERAL = 320,

    ENCRYPTED_WEB_PORT = 443,

    LAST_PRIVILEDGED_PORT_ID = 1024,

    // rtp audio bcast
    RTP_AUDIO_PORT = 5004,

    MDNS_PORT = 5353,

    LOCAL_WEB_PORT = 8080,

    // Session Announcement Protocol
    SAP_PORT_EVENT = 9875,

    UNKNOWN = 0xffff,
};
} // namespace iuring