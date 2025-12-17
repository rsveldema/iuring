#include <iuring/IOUringInterface.hpp>
#include <iuring/NetworkAdapter.hpp>
#include <iuring/ISocket.hpp>
#include <iuring/SocketFactoryImpl.hpp>

#include <slogger/TimeUtils.hpp>
#include <slogger/Logger.hpp>

using namespace std::chrono_literals;


namespace ping
{
bool connection_has_been_closed = false;

static void Usage()
{
    printf("Usage: --ping <ip>\n");
}

void handle_packet_sent(const std::shared_ptr<iuring::IOUringInterface>& io,
    const std::shared_ptr<iuring::ISocket>& socket,
    logging::ILogger& logger)
{
    io->submit_recv(socket, [&logger, io, socket](const iuring::ReceivedMessage& msg) {
        LOG_INFO(logger, "received: {}", msg.to_string());

        io->submit_close(socket, [&logger](const iuring::CloseResult& res) {
            LOG_INFO(logger, "connection closed: {}", res.status);
            assert(res.status == 0);
            connection_has_been_closed = true;
        });

        return iuring::ReceivePostAction::NONE;
    });
}

void handle_new_connection(const std::shared_ptr<iuring::IOUringInterface>& io,
    const std::shared_ptr<iuring::ISocket>& socket,
    logging::ILogger& logger)
{
    auto wi = io->ackuire_send_workitem(socket);
    auto& pkt = wi->get_send_packet();

    pkt.append("GET /posts/1 HTTP/1.1\r\n");
    pkt.append("Host: example.com\r\n");
    pkt.append("Accept: application/json\r\n");
    pkt.append("\r\n");

    wi->submit_stream_data([&logger, io, socket](const iuring::SendResult& result) {
        LOG_INFO(logger, "packet sent successfully: {}", result.status);
        handle_packet_sent(io, socket, logger);
    });
}

void do_http_ping(const iuring::IPAddress& ping_addr, logging::ILogger& logger,
    const std::string& interface_name, bool tune)
{
    auto port = iuring::SocketPortID::UNENCRYPTED_WEB_PORT;

    LOG_INFO(logger, "going to ping {}",
        ping_addr.to_human_readable_ip_string());

    iuring::SocketFactoryImpl factory;

    iuring::NetworkAdapter adapter(logger, interface_name, tune);
    auto io = iuring::IOUringInterface::create_impl(logger, adapter);
    io->init();

    auto socket = factory.create_impl(iuring::SocketType::IPV4_TCP,
        port, logger, iuring::SocketKind::UNICAST_CLIENT_SOCKET);
    io->submit_connect(
        socket, ping_addr, [io, socket, &logger](const iuring::ConnectResult& res) {
            assert(res.status == 0);
            handle_new_connection(io, socket, logger);
        });

    time_utils::Timeout timeout(20s);
    while (!connection_has_been_closed)
    {
        assert(!timeout.elapsed());
        io->poll_completion_queues();
    }
}
} // namespace ping


int main(int argc, char** argv)
{
    logging::DirectConsoleLogger logger(true, true, logging::LogOutput::CONSOLE);

    const std::string interface_name = "eth0";
    bool tune = true;

    std::optional<iuring::IPAddress> ping_addr_opt;
    for (int i = 0; i < argc; i++)
    {
        std::string arg{ argv[i] };
        if (arg == "--ping")
        {
            auto in_addr =
                iuring::IPAddress::string_to_ipv4_address(argv[i + 1], logger);
            iuring::IPAddress addr(
                in_addr, iuring::SocketPortID::ENCRYPTED_WEB_PORT);
            ping_addr_opt = addr;
        }
        else if (arg == "--no-tune")
        {
            tune = false;
        }
        else if (arg == "--help")
        {
            ping::Usage();
            return 1;
        }
    }

    if (! ping_addr_opt.has_value())
    {
        LOG_ERROR(logger, "missing --ping arg");
        return 1;
    }

    ping::do_http_ping(ping_addr_opt.value(), logger, interface_name, tune);
    return 0;
}
