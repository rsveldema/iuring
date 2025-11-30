#pragma once


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

#include <IPAddress.hpp>
#include <cassert>
#include <cstring>
#include <memory>

#include <ISocket.hpp>

namespace network
{
class AcceptResult;

class SocketImpl : public ISocket
{
private:
    SocketImpl(Logger& logger, const AcceptResult& new_conn);

    SocketImpl(
        SocketType type, SocketPortID port, Logger& logger, SocketKind kind);

public:
    static std::shared_ptr<SocketImpl> create(Logger& logger, const AcceptResult& new_conn);
    static std::shared_ptr<SocketImpl> create(SocketType type, SocketPortID port, Logger& logger, SocketKind kind);

    void dump_info();

    int mcast_bind() override;

    void join_multicast_group(const std::string& ip_address,
        const std::string& source_iface) override;
    void leave_multicast_group();

private:
    ip_mreq m_mreq{};

    void local_bind(SocketPortID port_id);
};

} // namespace network