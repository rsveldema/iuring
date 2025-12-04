#include <cassert>
#include <cerrno>
#include <cstring>
#include <array>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include <iuring/ILogger.hpp>
#include <iuring/IPAddress.hpp>

namespace iuring
{
std::string IPAddress::to_human_readable_ip_string() const
{
    std::array<char, 128> buffer;

    if (get_ipv4())
    {
        const char* source_name =
            inet_ntop(AF_INET, data_addr(), buffer.data(), buffer.size());
        if (!source_name)
        {
            source_name = "<INVALID>";
        }
        return std::string(source_name);
    }

    if (get_ipv6())
    {
        const char* source_name =
            inet_ntop(AF_INET6, data_addr(), buffer.data(), buffer.size());
        if (!source_name)
        {
            source_name = "<INVALID>";
        }
        return std::string(source_name);
    }
    return "?.?.?.?";
}


std::string IPAddress::to_human_readable_string() const
{
    std::array<char, 128> buffer;

    if (const auto* v = get_ipv4())
    {
        const char* source_name =
            inet_ntop(AF_INET, data_addr(), buffer.data(), buffer.size());
        if (!source_name)
        {
            source_name = "<INVALID>";
        }
        return std::string(source_name) + ", port " +
            std::to_string(htons(v->sin_port));
    }

    if (const auto* v = get_ipv6())
    {
        const char* source_name =
            inet_ntop(AF_INET6, data_addr(), buffer.data(), buffer.size());
        if (!source_name)
        {
            source_name = "<INVALID>";
        }
        return std::string(source_name) + ", port" +
            std::to_string(v->sin6_port);
    }
    return "?.?.?.?";
}

IPAddress create_sock_addr_in(
    const char* addr, const in_port_t port, logging::ILogger& logger)
{
    sockaddr_in dest_addr;
    memset(&dest_addr, 0, sizeof(dest_addr));
    dest_addr.sin_addr =
        iuring::IPAddress::string_to_ipv4_address(addr, logger);
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(port);
    return IPAddress(dest_addr);
}

in_addr IPAddress::string_to_ipv4_address(
    const std::string& _ip_address, logging::ILogger& logger)
{
    in_addr addr;

    std::string ip_address(_ip_address);
    if (StringUtils::ends_with(ip_address, "/32"))
    {
        ip_address = ip_address.substr(0, ip_address.length() - 3);
    }

    // fprintf(stderr, "using: {} instead of {}\n", ip_address.c_str(),
    // _ip_address.c_str());
    if (int ret = inet_pton(AF_INET, ip_address.c_str(), &(addr.s_addr));
        ret != 1)
    {
        if (ret < 0)
            perror("inet_pton - failed");
        else
            LOG_ERROR(logger, "invalid IP address: {}\n", ip_address.c_str());
        abort();
    }
    return addr;
}
} // namespace iuring