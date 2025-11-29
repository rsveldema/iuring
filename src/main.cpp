#include <IOUring.hpp>
#include <SocketImpl.hpp>
#include <TimeUtils.hpp>

using namespace std::chrono_literals;

static
void Usage()
{
    printf("Usage: --ping <ip>\n");
}

int main(int argc, char** argv) {
    Logger logger(true, true, LogOutput::CONSOLE);

    auto port = static_cast<network::SocketPortID>(80);
    const std::string interface_name = "eth0";
    bool tune = true;

    std::optional<network::IPAddress>  addr_opt;
    for (int i=0;i<argc;i++)
    {
        std::string arg{argv[i]};
        if (arg == "--ping")
        {
            auto in_addr = network::IPAddress::string_to_ipv4_address(argv[i+1], logger);
            network::IPAddress addr( in_addr, 80);
            addr_opt = addr;
        } else if (arg == "--no-tune") {
            tune= false;
        } else if (arg == "--help")
        {
            Usage();
            return 1;
        }
    }

    if (! addr_opt)
    {
        Usage();
        return 1;
    }

    LOG_INFO(logger, "going to ping %s\n", addr_opt.value().to_human_readable_ip_string().c_str());

    auto socket = std::make_shared<network::SocketImpl>(network::SocketType::IPV4_TCP, port, logger, network::SocketKind::CLIENT_SOCKET);

    auto io = std::make_shared<network::IOUring>(logger, interface_name, tune);
    io->init();
    io->submit_connect(socket, addr_opt.value());
    io->submit_all_requests();

    time_utils::Timeout timeout(10s);
    while (! timeout.elapsed())
    {
        io->poll_completion_queues();
    }
    return 0;
}
