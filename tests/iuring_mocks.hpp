#pragma once

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <iuring/IOUringInterface.hpp>
#include <iuring/ISocketFactory.hpp>

namespace iuring::mocks
{

    class Socket : public ISocket
    {
    public:
        Socket(SocketType type, SocketPortID port, logging::ILogger& logger,
            SocketKind kind, int fd)
            : ISocket(type, port, logger, kind, fd)
        {
        }

        MOCK_METHOD(int, mcast_bind, (), (override));
        MOCK_METHOD(void, join_multicast_group,
            (const std::string& ip_address, const std::string& source_iface),
            (override));
    };

    class SocketFactory : public ISocketFactory
    {
    public:
        std::shared_ptr<ISocket> create_impl(SocketType type, SocketPortID port,
            logging::ILogger& logger, SocketKind kind) override
        {
            // fd is not used for mocked sockets; use -1 as placeholder
            return std::make_shared<Socket>(type, port, logger, kind, -1);
        }

        std::shared_ptr<ISocket> create_impl(
            logging::ILogger& logger, const AcceptResult& res) override
        {
            // create a mock Socket reflecting the accepted connection
            const auto port = res.m_address.get_port();
            const auto type = SocketType::IPV4_TCP;
            return std::make_shared<Socket>(type, port, logger,
                SocketKind::SERVER_STREAM_SOCKET, res.m_new_fd);
        }
    };

    class WorkItem : public IWorkItem
    {
    public:
        MOCK_METHOD(SendPacket&, get_send_packet, (), (override));
        MOCK_METHOD(void, submit_packet, (const DatagramSendParameters& params, const send_callback_func_t& cb), (override));
        MOCK_METHOD(void, submit_stream_data, (const send_callback_func_t& cb), (override));
        MOCK_METHOD(std::shared_ptr<ISocket>, get_socket, (), (const, override));
    };

    class IOUring : public IOUringInterface
    {
    public:
        MOCK_METHOD(error::Error, init, (), (override));
        MOCK_METHOD(error::Error, poll_completion_queues, (), (override));
        MOCK_METHOD(void, submit_connect,
            (const std::shared_ptr<ISocket>& socket, const IPAddress& target,
                connect_callback_func_t handler),
            (override));
        MOCK_METHOD(void, submit_accept,
            (const std::shared_ptr<ISocket>& socket,
                accept_callback_func_t handler),
            (override));
        MOCK_METHOD(void, submit_recv,
            (const std::shared_ptr<ISocket>& socket,
                recv_callback_func_t handler),
            (override));
        MOCK_METHOD(std::shared_ptr<IWorkItem>, ackuire_send_workitem,
            (const std::shared_ptr<ISocket>& socket), (override));
        MOCK_METHOD(void, submit, (IWorkItem & item), (override));
        MOCK_METHOD(void, submit_close,
            (const std::shared_ptr<ISocket>& socket,
                close_callback_func_t handler),
            (override));

        MOCK_METHOD(void, resolve_hostname, (const std::string& hostname,
            const resolve_hostname_callback_func_t& handler), (override));
    };
} // namespace mocks