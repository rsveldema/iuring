#pragma once

#include <map>

#include "IPAddress.hpp"
#include "UringDefs.hpp"
#include "Logger.hpp"
#include "WorkPool.hpp"

namespace network
{
class IOUringInterface
{
public:
    IOUringInterface(Logger& logger, const std::string& interface_name, bool tune)
        : m_logger(logger), m_pool(logger), m_interface_name(interface_name), m_tune(tune)
    {
    }

    WorkPool& get_pool() { return m_pool; }

    virtual Error init();
    virtual Error poll_completion_queues() = 0;
    virtual void re_submit(WorkItem& item) = 0;
    virtual void submit_connect(const std::shared_ptr<ISocket>& socket, const IPAddress& target) = 0;
    virtual void submit_all_requests() = 0;



    /** @returns aa:bb:cc:dd:ee:ff
     */
    std::string get_my_mac_address();

    const std::string get_interface_ip4() const
    {
        return m_interface_ip4;
    }

    const std::string get_interface_name() const
    {
        return m_interface_name;
    }

    Logger& get_logger()
    {
        return m_logger;
    }

private:
    Logger& m_logger;
    WorkPool m_pool;
    std::string m_interface_ip4;
    std::string m_interface_ip6;
    std::string m_interface_name;
    bool m_tune = true;
    std::optional<std::string> mac_opt;

protected:
    std::map<UringFeature, bool> m_features;
    bool m_initialized = false;

    void tune();

    void set_interface_ip4(const std::string& ip) {
        m_interface_ip4 = ip;
        LOG_INFO(get_logger(), "interface IP4 set to " + ip);
    }

    void set_interface_ip6(const std::string& ip) {
        m_interface_ip6 = ip;
        LOG_INFO(get_logger(), "interface IP6 set to " + ip);
    }

    virtual bool try_get_interface_ip();
    virtual bool retrieve_interface_ip();
};

Error errno_to_error(int err);

} // namespace network