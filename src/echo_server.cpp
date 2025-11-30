#include <IOUring.hpp>
#include <SocketImpl.hpp>
#include <TimeUtils.hpp>

using namespace std::chrono_literals;

namespace server
{
static void Usage()
{
    printf("Usage: --ping <ip>\n");
}

bool should_quit = false;

void handle_new_connection(const network::AcceptResult& res,
    const std::shared_ptr<network::IOUringInterface>& io,
    Logger& logger)
{
    auto socket = network::SocketImpl::create(logger, res);

    io->submit_recv(socket, [](const network::ReceivedMessage& msg) {
        fprintf(stderr, "received: %s\n", msg.to_string().c_str());
        return network::ReceivePostAction::RE_SUBMIT;
    });
}

void do_webserver(Logger& logger, const std::string& interface_name, bool tune)
{
    auto port = network::SocketPortID::LOCAL_WEB_PORT;

    LOG_INFO(logger, "going to do a simple websever\n");

    auto socket = network::SocketImpl::create(network::SocketType::IPV4_TCP,
            port, logger, network::SocketKind::SERVER_STREAM_SOCKET);

    network::NetworkAdapter adapter(logger, interface_name, tune);
    auto io = network::IOUring::create(logger, adapter);
    io->init();

    io->submit_accept(
        socket, [io, socket, &logger](const network::AcceptResult& res) {
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
    Logger logger(true, true, LogOutput::CONSOLE);

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

    server::do_webserver(logger, interface_name, tune);
    return 0;
}