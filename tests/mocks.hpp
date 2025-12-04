#pragma once

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <iuring/IOUringInterface.hpp>

namespace iuring
{
namespace mocks
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
        MOCK_METHOD(
            std::optional<MacAddress>, get_my_mac_address, (), (override));
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
        MOCK_METHOD(std::shared_ptr<IWorkItem>, submit_send,
            (const std::shared_ptr<ISocket>& socket), (override));
        MOCK_METHOD(void, submit, (IWorkItem & item), (override));
        MOCK_METHOD(void, submit_close,
            (const std::shared_ptr<ISocket>& socket,
                close_callback_func_t handler),
            (override));
    };
} // namespace mocks
} // namespace iuring