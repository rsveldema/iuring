#include <chrono>
#include <ifaddrs.h>
#include <thread>

#include "IOUring.hpp"

#include <slogger/ShellUtils.hpp>

#include <iuring/IOUringInterface.hpp>

using namespace std::chrono_literals;

namespace iuring
{
void NetworkAdapter::init()
{
    retrieve_interface_ip();
    tune();

    char hostname[256];
    if (gethostname(hostname, sizeof(hostname)) != 0)
    {
        LOG_ERROR(get_logger(), "gethostname() failed");
        abort();
    }
    m_hostname = std::string(hostname);
}


void NetworkAdapter::tune()
{
    if (! m_tune)
    {
        LOG_INFO(get_logger(), "not tuning interface settings");
        return;
    }
    shell::run_cmd("ethtool -C " + m_interface_name + " tx-usecs 1",
        get_logger(), shell::RunOpt::LOG_ERROR_AS_WARNING);
    shell::run_cmd("ethtool -C " + m_interface_name + " rx-usecs 1",
        get_logger(), shell::RunOpt::ABORT_ON_ERROR);
    shell::run_cmd("ethtool -C " + m_interface_name + " rx-frames 1",
        get_logger(), shell::RunOpt::ABORT_ON_ERROR);
    shell::run_cmd("ethtool -C " + m_interface_name + " tx-frames 1",
        get_logger(), shell::RunOpt::ABORT_ON_ERROR);
}


bool NetworkAdapter::try_get_interface_ip()
{
    assert(get_interface_name() != "");

    bool success = false;
    ifaddrs* ifaddr = nullptr;
    if (getifaddrs(&ifaddr) == -1)
    {
        perror("getifaddrs");
        abort();
    }

    for (ifaddrs* ifa = ifaddr; ifa != nullptr; ifa = ifa->ifa_next)
    {
        if (ifa->ifa_addr == nullptr)
            continue;

        const auto family = ifa->ifa_addr->sa_family;
        if (ifa->ifa_name != get_interface_name())
        {
            LOG_INFO(get_logger(), "skip: interface {}", ifa->ifa_name);
            continue;
        }

        LOG_INFO(get_logger(), "FOUND INTERFACE: {} {} ({})", ifa->ifa_name,
            (family == AF_PACKET)    ? "AF_PACKET" :
                (family == AF_INET)  ? "AF_INET" :
                (family == AF_INET6) ? "AF_INET6" :
                                       "???",
            family);

        switch (family)
        {
        case AF_INET: {
            IPAddress ip(*(sockaddr_in*) ifa->ifa_addr);
            set_interface_ip4(ip.to_human_readable_ip_string());
            success = true;
            break;
        }
        case AF_INET6: {
            IPAddress ip(*(sockaddr_in6*) ifa->ifa_addr);
            set_interface_ip6(ip.to_human_readable_ip_string());
            break;
        }
        default:
            break;
        }
    }

    freeifaddrs(ifaddr);
    return success;
}


void NetworkAdapter::retrieve_interface_ip()
{
    // try_get_interface_ip();
    // set_interface_ip4("192.168.1.130");

    while (! get_interface_ip4().has_value())
    {
        try_get_interface_ip();
        std::this_thread::sleep_for(1s);
    }
}

std::optional<MacAddress> NetworkAdapter::get_my_mac_address()
{
    if (mac_opt)
    {
        return *mac_opt;
    }

    auto filename = "/sys/class/net/" + get_interface_name() + "/address";
    FILE* f = fopen(filename.c_str(), "r");
    if (!f)
    {
        LOG_ERROR(get_logger(), "failed to open {}", filename);
        abort();
        return std::nullopt;
    }

    std::array<char, 128> buffer;
    buffer.fill(0);

    int num_bytes_read = 0;
    for (int i = 0; i < 10; i++)
    {
        num_bytes_read = fread(buffer.data(), 1, buffer.size(), f);
        if (num_bytes_read > 0)
        {
            break;
        }
    }
    assert(num_bytes_read > 0);
    fclose(f);
    mac_opt = MacAddress{ StringUtils::trim(buffer.data()) };
    return *mac_opt;
}


} // namespace iuring