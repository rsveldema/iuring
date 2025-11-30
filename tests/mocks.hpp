#pragma once

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <IOUringInterface.hpp>

namespace network
{
namespace mocks
{
    class WorkItem : public IWorkItem
    {
    public:
        MOCK_METHOD(SendPacket&, get_send_packet, (), (override));
        MOCK_METHOD(void, submit, (const send_callback_func_t& cb), (override));
    };

    class IOUring : public IOUringInterface
    {
    public:
        MOCK_METHOD(Error, init, (), (override));
        MOCK_METHOD(
            std::optional<MacAddress>, get_my_mac_address, (), (override));
        MOCK_METHOD(Error, poll_completion_queues, (), (override));
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
} // namespace network