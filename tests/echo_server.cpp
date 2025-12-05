#include <iuring/IOUringInterface.hpp>
#include <iuring/ISocket.hpp>

#include <slogger/TimeUtils.hpp>
#include <slogger/Logger.hpp>

using namespace std::chrono_literals;

namespace server
{
static void Usage()
{
    printf("Usage: --ping <ip>\n");
}

bool should_quit = false;

void handle_new_connection(const iuring::AcceptResult& res,
    const std::shared_ptr<iuring::IOUringInterface>& io,
    logging::ILogger& logger)
{
    auto socket = iuring::ISocket::create_impl(logger, res);

    io->submit_recv(socket, [&](const iuring::ReceivedMessage& msg) {
        LOG_INFO(logger, "received: {}", msg.to_string());
        return iuring::ReceivePostAction::RE_SUBMIT;
    });
}

void do_echo_server(logging::ILogger& logger, const std::string& interface_name, bool tune)
{
    auto port = iuring::SocketPortID::LOCAL_WEB_PORT;

    LOG_INFO(logger, "going to do a simple echo server");

    auto socket = iuring::ISocket::create_impl(iuring::SocketType::IPV4_TCP,
            port, logger, iuring::SocketKind::SERVER_STREAM_SOCKET);

    iuring::NetworkAdapter adapter(logger, interface_name, tune);
    auto io = iuring::IOUringInterface::create_impl(logger, adapter);
    io->init();

    io->submit_accept(
        socket, [io, socket, &logger](const iuring::AcceptResult& res) {
            assert(res.m_new_fd != 0);

            handle_new_connection(res, io, logger);
        });

    LOG_INFO(logger, "waiting for new requests");
    while (!should_quit)
    {
        io->poll_completion_queues();
    }
    LOG_INFO(logger, "exiting...");
}
} // namespace server


int main(int argc, char** argv)
{
    logging::Logger logger(true, true, logging::LogOutput::CONSOLE);

    const std::string interface_name = "eth0";
    bool tune = true;

    for (int i = 0; i < argc; i++)
    {
        std::string arg{ argv[i] };
        if (arg == "--no-tune")
        {
            tune = false;
        }
        else if (arg == "--help")
        {
            server::Usage();
            return 1;
        }
    }

    server::do_echo_server(logger, interface_name, tune);
    return 0;
}