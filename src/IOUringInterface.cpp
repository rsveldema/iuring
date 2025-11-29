#include <IOUring.hpp>
#include <ShellUtils.hpp>
#include <chrono>
#include <ifaddrs.h>
#include <thread>

using namespace std::chrono_literals;

namespace network
{

Error errno_to_error(int err)
{
    switch (err)
    {
    case 0:
        return Error::OK;
        return Error::RANGE;

    default:
        break;
    }
    return Error::UNKNOWN;
}

Error IOUringInterface::init()
{
    retrieve_interface_ip();
    tune();
    return Error::OK;
}


void IOUringInterface::tune()
{
    if (! m_tune)
    {
        LOG_INFO(get_logger(), "not tuning interface settings\n");
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


bool IOUringInterface::try_get_interface_ip()
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
            LOG_INFO(get_logger(), "skip: interface %s", ifa->ifa_name);
            continue;
        }

        LOG_INFO(get_logger(), "FOUND INTERFACE: %-8s %s (%d)\n", ifa->ifa_name,
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

bool IOUringInterface::retrieve_interface_ip()
{
    // try_get_interface_ip();
    // set_interface_ip4("192.168.1.130");

    while (get_interface_ip4() == "")
    {
        try_get_interface_ip();
        std::this_thread::sleep_for(1s);
    }
    return true;
}

std::string IOUringInterface::get_my_mac_address()
{
    if (mac_opt)
    {
        return *mac_opt;
    }

    std::array<char, 128> buffer;
    buffer.fill(0);
    snprintf(buffer.data(), buffer.size(), "/sys/class/net/%s/address",
        get_interface_name().c_str());
    FILE* f = fopen(buffer.data(), "r");
    if (!f)
    {
        fprintf(stderr, "failed to open %s\n", buffer.data());
        abort();
        return "";
    }
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
    mac_opt = StringUtils::trim(buffer.data());
    return *mac_opt;
}


} // namespace network