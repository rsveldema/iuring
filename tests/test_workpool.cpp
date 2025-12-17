#include <gtest/gtest.h>

#include "iuring_mocks.hpp"

#include "../src/WorkPool.hpp"

#include <slogger/Logger.hpp>

using testing::_;


namespace Tests
{
class TestWorkPool : public testing::Test
{
public:
    logging::DirectConsoleLogger logger{ true, true, logging::LogOutput::CONSOLE };
    iuring::WorkPool wp{ logger };

    std::shared_ptr<iuring::mocks::Socket> socket =
        std::make_shared<iuring::mocks::Socket>(iuring::SocketType::IPV4_TCP,
            iuring::SocketPortID::LOCAL_WEB_PORT, logger,
            iuring::SocketKind::UNICAST_CLIENT_SOCKET, 42);

    std::shared_ptr<iuring::mocks::IOUring> io =
        std::make_shared<iuring::mocks::IOUring>();
    bool seen_submit = false;
    bool seen_callback = false;
};

TEST_F(TestWorkPool, test_wp_accept)
{
    ASSERT_EQ(socket->get_fd(), 42);

    EXPECT_CALL(*io, submit(_)).WillOnce([this](iuring::IWorkItem& item) {
        ASSERT_EQ(item.get_type(), iuring::IWorkItem::Type::ACCEPT);
        seen_submit = true;

        auto* k = dynamic_cast<iuring::WorkItem *>(&item);
        ASSERT_NE(k, nullptr);

        iuring::AcceptResult ret;
        ret.m_new_fd = 12345;
        k->call_accept_callback(ret);
    });

    auto item = wp.alloc_accept_work_item(
        socket, io,
        [this](const iuring::AcceptResult& result) {
            ASSERT_EQ(result.m_new_fd, 12345);
            seen_callback = true;
        },
        "test-accept");

    ASSERT_NE(item, nullptr);
    ASSERT_TRUE(seen_submit);
    ASSERT_TRUE(seen_callback);
    ASSERT_EQ(item->get_type(), iuring::IWorkItem::Type::ACCEPT);
    ASSERT_EQ(socket, item->get_socket());
    ASSERT_EQ(item->get_type_str(), std::string("accept"));

    // the item should be registered:
    ASSERT_EQ(wp.get_work_item(item->get_id()), item);
    wp.free_work_item(item->get_id());

    // after freeing it, we get nullptr back:
    ASSERT_EQ(wp.get_work_item(item->get_id()), nullptr);

    // double free causes an assert (in debug build...):
    ASSERT_EXIT(wp.free_work_item(item->get_id()),
        ::testing::KilledBySignal(SIGABRT), "work_item != nullptr");
}

TEST_F(TestWorkPool, test_wp_close)
{
    EXPECT_CALL(*io, submit(_)).WillOnce([this](iuring::IWorkItem& item) {
        ASSERT_EQ(item.get_type(), iuring::IWorkItem::Type::CLOSE);
        seen_submit = true;

        auto* k = dynamic_cast<iuring::WorkItem *>(&item);
        ASSERT_NE(k, nullptr);
        k->call_close_callback(12321);
    });

    auto item = wp.alloc_close_work_item(
        socket, io,
        [](const iuring::CloseResult& result) {
            ASSERT_EQ(result.status, 12321);
        },
        "test-close");

    ASSERT_NE(item, nullptr);
    ASSERT_TRUE(seen_submit);
    ASSERT_EQ(socket, item->get_socket());
    ASSERT_EQ(item->get_type_str(), std::string("close"));
}

TEST_F(TestWorkPool, test_wp_connect)
{
    EXPECT_CALL(*io, submit(_)).WillOnce([this](iuring::IWorkItem& item) {
        ASSERT_EQ(item.get_type(), iuring::IWorkItem::Type::CONNECT);
        seen_submit = true;

        auto* k = dynamic_cast<iuring::WorkItem *>(&item);
        ASSERT_NE(k, nullptr);
        iuring::ConnectResult ret;
        ret.status = 12321;
        ret.m_address = iuring::IPAddress(
            iuring::IPAddress::string_to_ipv4_address("9.8.7.6", logger),
            iuring::SocketPortID::ENCRYPTED_WEB_PORT
        );
        k->call_connect_callback(ret);
    });

    auto item = wp.alloc_connect_work_item(
        iuring::IPAddress(  iuring::IPAddress::string_to_ipv4_address("1.2.3.4", logger),
                iuring::SocketPortID::ENCRYPTED_WEB_PORT ),
        socket, io,
        [](const iuring::ConnectResult& result) {
            ASSERT_EQ(result.status, 12321);
            ASSERT_EQ(result.m_address.to_human_readable_ip_string(), "9.8.7.6");
        },
        "test-conn");

    ASSERT_NE(item, nullptr);
    ASSERT_TRUE(seen_submit);
    ASSERT_EQ(socket, item->get_socket());
    ASSERT_EQ(item->get_type_str(), std::string("connect"));
}

} // namespace Tests