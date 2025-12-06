#pragma once

/**
 * @file IPAddress.hpp
 * @brief Defines the IPAddress class for handling IPv4 and IPv6 addresses.
 */


#include <cassert>
#include <cstring>
#include <net/if.h>
#include <netinet/in.h>
#include <optional>
#include <poll.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <unistd.h>

#include <slogger/ILogger.hpp>
#include <slogger/StringUtils.hpp>

#include <iuring/NetworkProtocols.hpp>

namespace iuring
{
class IPAddress
{
public:
    IPAddress() = default;

    explicit IPAddress(const in_addr& sa, SocketPortID port)
    {
        sockaddr_in sa_in;
        memset(&sa_in, 0, sizeof(sa));

        sa_in.sin_family = AF_INET;
        sa_in.sin_port = htons( static_cast<uint16_t>(port));
        sa_in.sin_addr = sa;

        m_in4 = sa_in;
    }

    explicit IPAddress(const in6_addr& sa, SocketPortID port)
    {
        sockaddr_in6 sa_in;
        memset(&sa_in, 0, sizeof(sa));

        sa_in.sin6_family = AF_INET6;
        sa_in.sin6_port = htons(static_cast<uint16_t>(port));
        sa_in.sin6_addr = sa;

        m_in6 = sa_in;
    }

    explicit IPAddress(const sockaddr_in& sa)
        : m_in4(sa)
    {
    }

    explicit IPAddress(const sockaddr_in6& sa)
        : m_in6(sa)
    {
    }

    IPAddress(const sockaddr_storage& sa, socklen_t len)
        : m_in4(len == sizeof(sockaddr_in) ?
                  std::nullopt :
                  std::optional<sockaddr_in>(*(sockaddr_in*) &sa))
        , m_in6(len == sizeof(sockaddr_in6) ?
                  std::nullopt :
                  std::optional<sockaddr_in6>(*(sockaddr_in6*) &sa))
    {
        assert((len == sizeof(sockaddr_in)) || (len == sizeof(sockaddr_in6)));
    }

    void set_port(SocketPortID port)
    {
        if (auto* a = get_mut_ipv4())
        {
            a->sin_port = htons(static_cast<uint16_t>(port));
            return;
        }

        if (auto* a = get_mut_ipv6())
        {
            a->sin6_port = htons(static_cast<uint16_t>(port));
            return;
        }
        abort();
    }

    SocketPortID get_port() const
    {
        if (auto* a = get_ipv4())
        {
            const auto p = htons(static_cast<uint16_t>(a->sin_port));
            return static_cast<SocketPortID>(p);
        }

        if (auto* a = get_ipv6())
        {
            const auto p = htons(static_cast<uint16_t>(a->sin6_port));
            return static_cast<SocketPortID>(p);
        }
        abort();
    }


    bool valid() const
    {
        if (get_ipv4())
            return true;
        if (get_ipv6())
            return true;
        return false;
    }

    /** IP address and port */
    std::string to_human_readable_string() const;

    /** just the IP address is returned */
    std::string to_human_readable_ip_string() const;

    const void* data_sockaddr() const
    {
        if (const auto* a = get_ipv4())
            return a;
        if (const auto* b = get_ipv6())
            return b;
        abort();
    }

    socklen_t size_sockaddr() const
    {
        if (const auto* a = get_ipv4())
            return sizeof(*a);
        if (const auto* b = get_ipv6())
            return sizeof(*b);
        abort();
    }

    const void* data_addr() const
    {
        if (const auto* a = get_ipv4())
            return &a->sin_addr.s_addr;
        if (const auto* b = get_ipv6())
            return &b->sin6_addr;
        abort();
    }

    size_t size_addr() const
    {
        if (const auto* a = get_ipv4())
            return sizeof(a->sin_addr);
        if (const auto* b = get_ipv6())
            return sizeof(b->sin6_addr);
        abort();
    }

    /** returns null if not ipv4 */
    const sockaddr_in* get_ipv4() const
    {
        if (m_in4)
        {
            return &*m_in4;
        }

        return nullptr;
    }

    /** returns null if not ipv6 */
    const sockaddr_in6* get_ipv6() const
    {
        if (m_in6)
        {
            return &*m_in6;
        }
        return nullptr;
    }

    sockaddr_in* get_mut_ipv4()
    {
        if (m_in4)
        {
            return &*m_in4;
        }

        return nullptr;
    }


    sockaddr_in6* get_mut_ipv6()
    {
        if (m_in6)
        {
            return &*m_in6;
        }
        return nullptr;
    }

    uint64_t get_hash() const
    {
        if (m_in4)
        {
            return *(uint32_t*) &m_in4->sin_addr;
        }
        else if (m_in6)
        {
            const uint32_t* a = m_in6->sin6_addr.__in6_u.__u6_addr32;

            const uint64_t ret = (((uint64_t) a[0]) << 0) |
                (((uint64_t) a[1]) << 32) | (((uint64_t) a[2]) << 0) |
                (((uint64_t) a[3]) << 32);
            return ret;
        }

        abort();
    }

    bool operator == (const IPAddress& other) const
    {
        if (m_in4.has_value() and other.m_in4.has_value())
        {
            const auto& v1 = m_in4.value();
            const auto& v2 = other.m_in4.value();
            return
                v1.sin_port == v2.sin_port &&
                memcmp(&v1.sin_addr, &v2.sin_addr, sizeof(v1.sin_addr)) == 0 &&
                v1.sin_family == v2.sin_family;
        }
        if (m_in6.has_value() and other.m_in6.has_value())
        {
            const auto& v1 = m_in6.value();
            const auto& v2 = other.m_in6.value();
            return
                v1.sin6_port == v2.sin6_port &&
                memcmp(&v1.sin6_addr, &v2.sin6_addr, sizeof(v1.sin6_addr)) == 0 &&
                v1.sin6_family == v2.sin6_family;
        }
        return false;
    }


    bool operator<(const IPAddress& addr) const
    {
        return get_hash() < addr.get_hash();
    }

public:
    static in_addr string_to_ipv4_address(
        const std::string& _ip_address, logging::ILogger& logger);

private:
    std::optional<sockaddr_in> m_in4;
    std::optional<sockaddr_in6> m_in6;
};


/** util func for converting a 'a.b.c.d' IP address and
 * port to an IPAddress object
 */
IPAddress create_sock_addr_in(
    const char* addr, const SocketPortID port, logging::ILogger& logger);

} // namespace iuring




template <>
struct std::formatter<iuring::IPAddress> {
    constexpr auto parse(std::format_parse_context& ctx) {
        return ctx.begin();
    }

    auto format(const iuring::IPAddress& c, std::format_context& ctx) const {
        return std::format_to(ctx.out(), "{}", c.to_human_readable_string());
    }
};

