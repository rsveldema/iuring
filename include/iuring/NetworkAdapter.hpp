#pragma once

#include <optional>
#include <string>

#include <slogger/ILogger.hpp>

#include <iuring/IPAddress.hpp>

#include "MacAddress.hpp"

namespace iuring
{
class NetworkAdapter
{
public:
    NetworkAdapter(
        logging::ILogger& logger, const std::string& interface_name, bool tune)
        : m_logger(logger)
        , m_interface_name(interface_name)
        , m_tune(tune)
    {
    }

    void init();

    void tune();

    void set_interface_name(const std::string& interface_name)
    {
        m_interface_name = interface_name;
        LOG_INFO(get_logger(), "interface name set to {}", interface_name);
    }


    const std::string& get_hostname() const
    {
        return m_hostname;
    }

    void set_interface_ip4(const iuring::IPAddress& ip)
    {
        m_interface_ip4 = ip;
        LOG_INFO(get_logger(), "interface IP4 set to {}", ip);
    }

    void set_interface_ip6(const iuring::IPAddress& ip)
    {
        m_interface_ip6 = ip;
        LOG_INFO(get_logger(), "interface IP6 set to {}", ip);
    }

    /**
     * returns eth line eth0, eth1, etc. or wlan0
     */
    const std::string& get_interface_name() const
    {
        return m_interface_name;
    }

    std::optional<MacAddress> get_my_mac_address();

    /** @return the IP address we're currently bound to on our selected
     * interface (eth0 in '--dev eth0')
     */
    const std::optional<iuring::IPAddress> get_interface_ip4() const
    {
        return m_interface_ip4;
    }

    /** @return the IP address we're currently bound to on our selected
     * interface (eth0 in '--dev eth0')
     */
    const std::optional<iuring::IPAddress> get_interface_ip6() const
    {
        return m_interface_ip6;
    }

private:
    std::string m_hostname;
    logging::ILogger& m_logger;

    std::optional<iuring::IPAddress> m_interface_ip4;
    std::optional<iuring::IPAddress> m_interface_ip6;

    // eth0
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
} // namespace iuring