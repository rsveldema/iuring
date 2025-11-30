#pragma once

#include <Logger.hpp>
#include <Error.hpp>

#include <string>
#include <optional>

namespace network
{

class NetworkAdapter
{
public:
    NetworkAdapter(Logger& logger, const std::string& interface_name, bool tune)
    : m_logger(logger),
      m_interface_name(interface_name)
    , m_tune(tune)
    {}

    Error init();

    void tune();

    void set_interface_ip4(const std::string& ip)
    {
        m_interface_ip4 = ip;
        LOG_INFO(get_logger(), "interface IP4 set to " + ip);
    }

    void set_interface_ip6(const std::string& ip)
    {
        m_interface_ip6 = ip;
        LOG_INFO(get_logger(), "interface IP6 set to " + ip);
    }

    const std::string& get_interface_name() const
    {
        return m_interface_name;
    }

    bool try_get_interface_ip();
    bool retrieve_interface_ip();


    std::string get_my_mac_address();

    const std::string get_interface_ip4() const{
        return m_interface_ip4;
    }

    const std::string get_interface_ip6() const{
        return m_interface_ip6;
    }

private:
    Logger& m_logger;

    std::string m_interface_ip4;
    std::string m_interface_ip6;
    std::string m_interface_name;
    bool m_tune = true;
    std::optional<std::string> mac_opt;

    Logger& get_logger()
    {
        return m_logger;
    }
};
}