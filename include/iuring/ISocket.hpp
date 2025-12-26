#pragma once

/**
 * @file ISocket.hpp
 * @brief Defines the ISocket interface for network sockets.
 *
 * This interface provides the basic functionalities for different types of
 * network sockets, including multicast binding and joining multicast groups.
 * The real implementations will derive from this interface.
 * The main implementation is in SocketImpl.hpp
 */

#include <net/if.h>
#include <netinet/in.h>
#include <optional>
#include <poll.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <unistd.h>
#include <queue>

#include <cassert>
#include <cstring>

#include <slogger/ILogger.hpp>
#include <slogger/StringUtils.hpp>

#include <iuring/IPAddress.hpp>
#include <iuring/CompletionCallbacks.hpp>

namespace iuring
{
class AcceptResult;
class IOUringInterface;

enum class SocketState
{
 INITIAL,
 HANDSHAKE,
 READY,
 CLOSED
};

class ISocket : public std::enable_shared_from_this<ISocket>
{
public:
    ISocket(SocketType type, SocketPortID port, logging::ILogger& logger,
        SocketKind kind, int fd)
        : m_type(type)
        , m_port(port)
        , m_logger(logger)
        , m_kind(kind)
        , m_fd(fd)
    {
    }

    SocketState get_state() const { return m_state; }
    void set_state(SocketState s) { m_state = s; }

    virtual void send(const std::shared_ptr<iuring::IOUringInterface>& io,
        const std::string& reply_msg, const send_callback_func_t &cb) = 0;

    virtual int mcast_bind() = 0;

    virtual void join_multicast_group(
        const std::string& ip_address, const std::string& source_iface) = 0;

    virtual ~ISocket() = default;

    int get_fd() const
    {
        return m_fd;
    }


    SocketPortID get_port() const
    {
        return m_port;
    }

    SocketKind get_kind() const
    {
        return m_kind;
    }

    bool is_stream() const
    {
        switch (m_type)
        {
        case SocketType::IPV4_TCP:
        case SocketType::IPV6_TCP:
            return true;
        case SocketType::UNKNOWN:
        case SocketType::IPV4_UDP:
        case SocketType::IPV6_UDP:
            return false;
        }
        return false;
    }

    SocketType get_type() const
    {
        return m_type;
    }

    logging::ILogger& get_logger()
    {
        return m_logger;
    }

    void push_encrypted_data(const iuring::ReceivedMessage& data);
    bool has_encrypted_data() const { return !m_encrypted_data.empty(); }
    size_t num_bytes_encrypted_data() const { return m_encrypted_data.size(); }
    size_t copy_out_encrypted_data(uint8_t* buf, size_t len);

private:
    SocketType m_type;
    SocketPortID m_port;
    logging::ILogger& m_logger;
    SocketKind m_kind;
    int m_fd;
    SocketState m_state = SocketState::INITIAL;
    std::queue<uint8_t> m_encrypted_data;

private:
    friend class SocketFactoryImpl;

    static std::shared_ptr<ISocket> create_impl(SocketType type,
        SocketPortID port, logging::ILogger& logger, SocketKind kind);

    static std::shared_ptr<ISocket> create_impl(
        logging::ILogger& logger, const AcceptResult& res);
};

} // namespace iuring