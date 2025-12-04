#pragma once

#include <string>
#include <optional>

#include "ILogger.hpp"
#include "MacAddress.hpp"

namespace iuring
{
class NetworkAdapter
{
public:
    NetworkAdapter(logging::ILogger& logger, const std::string& interface_name, bool tune)
    : m_logger(logger),
      m_interface_name(interface_name)
    , m_tune(tune)
    {}

    void init();

    void tune();

    void set_interface_ip4(const std::string& ip)
    {
        m_interface_ip4 = ip;
        LOG_INFO(get_logger(), "interface IP4 set to {}", ip);
    }

    void set_interface_ip6(const std::string& ip)
    {
        m_interface_ip6 = ip;
        LOG_INFO(get_logger(), "interface IP6 set to {}", ip);
    }

    const std::string& get_interface_name() const
    {
        return m_interface_name;
    }

   std::optional<MacAddress> get_my_mac_address();

    const std::optional<std::string> get_interface_ip4() const{
        return m_interface_ip4;
    }

    const std::optional<std::string> get_interface_ip6() const{
        return m_interface_ip6;
    }

private:
    logging::ILogger& m_logger;

    std::optional<std::string> m_interface_ip4;
    std::optional<std::string> m_interface_ip6;
    std::string m_interface_name;
    bool m_tune = true;
    std::optional<MacAddress> mac_opt;

    bool try_get_interface_ip();
    void retrieve_interface_ip();

    logging::ILogger& get_logger()
    {
        return m_logger;
    }
};
}