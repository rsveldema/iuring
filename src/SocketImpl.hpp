#pragma once


#include <net/if.h>
#include <netinet/in.h>
#include <optional>
#include <poll.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <unistd.h>

#include <cassert>
#include <cstring>
#include <memory>

#include <slogger/ILogger.hpp>
#include <slogger/StringUtils.hpp>

#include <iuring/IPAddress.hpp>
#include <iuring/ISocket.hpp>

namespace iuring
{
class AcceptResult;

class SocketImpl : public ISocket
{
private:
    SocketImpl(logging::ILogger& logger, const AcceptResult& new_conn);

    SocketImpl(
        SocketType type, SocketPortID port, logging::ILogger& logger, SocketKind kind);

public:
    static std::shared_ptr<SocketImpl> create(logging::ILogger& logger, const AcceptResult& new_conn);
    static std::shared_ptr<SocketImpl> create(SocketType type, SocketPortID port, logging::ILogger& logger, SocketKind kind);

    void dump_info();

    void send(const std::shared_ptr<iuring::IOUringInterface>& io,
        const std::string& reply_msg, const iuring::send_callback_func_t &cb) override;

    int mcast_bind() override;

    void join_multicast_group(const std::string& ip_address,
        const std::string& source_iface) override;
    void leave_multicast_group();

private:
    ip_mreq m_mreq{};

    void local_bind(SocketPortID port_id);
};

} // namespace iuring