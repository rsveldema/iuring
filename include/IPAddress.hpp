#pragma once


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

#include <Logger.hpp>
#include <StringUtils.hpp>
#include <NetworkProtocols.hpp>

namespace network
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

    bool valid() const
    {
        if (get_ipv4())
            return true;
        if (get_ipv6())
            return true;
        return false;
    }

    std::string to_human_readable_string() const;
    std::string to_human_readable_ip_string() const;

    const void* data_sockaddr() const
    {
        if (auto* a = get_ipv4())
            return a;
        if (auto* b = get_ipv6())
            return b;
        abort();
    }

    socklen_t size_sockaddr() const
    {
        if (auto* a = get_ipv4())
            return sizeof(*a);
        if (auto* b = get_ipv6())
            return sizeof(*b);
        abort();
    }

    const void* data_addr() const
    {
        if (auto* a = get_ipv4())
            return &a->sin_addr.s_addr;
        if (auto* b = get_ipv6())
            return &b->sin6_addr;
        abort();
    }

    size_t size_addr() const
    {
        if (auto* a = get_ipv4())
            return sizeof(a->sin_addr);
        if (auto* b = get_ipv6())
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


    bool operator<(const IPAddress& addr) const
    {
        return get_hash() < addr.get_hash();
    }

public:
    static in_addr string_to_ipv4_address(
        const std::string& _ip_address, Logger& logger);

private:
    std::optional<sockaddr_in> m_in4;
    std::optional<sockaddr_in6> m_in6;
};


} // namespace network