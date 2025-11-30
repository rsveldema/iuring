#include <IOUringInterface.hpp>

namespace network
{
const char* WorkItem::type_to_string(Type t)
{
    switch (t)
    {
    case Type::ACCEPT:
        return "accept";
    case Type::CONNECT:
        return "connect";
    case Type::RECV:
        return "recv";
    case Type::SEND:
        return "send";
    case Type::UNKNOWN:
        return "unknown";
    case Type::CLOSE:
        return "close";
    }
    return "<unknown type of work item>";
}


void WorkItem::submit(const recv_callback_func_t& cb)
{
    m_callback = cb;
    m_work_type = Type::RECV;
    m_io_ring->submit(*this);
}

void WorkItem::submit(const send_callback_func_t& cb)
{
    m_callback = cb;
    m_work_type = Type::SEND;
    m_io_ring->submit(*this);
}

void WorkItem::submit(const accept_callback_func_t& cb)
{
    m_callback = cb;
    m_work_type = Type::ACCEPT;
    m_io_ring->submit(*this);
}

void WorkItem::submit(
    const IPAddress& target, const connect_callback_func_t& cb)
{
    LOG_INFO(get_logger(), "connecting to %s\n",
        target.to_human_readable_ip_string().c_str())

    m_connect_sock_len = target.size_sockaddr();
    assert(sizeof(m_buffer_for_uring) >= m_connect_sock_len);
    memcpy(&m_buffer_for_uring, target.data_sockaddr(), m_connect_sock_len);

    m_callback = cb;
    m_work_type = Type::CONNECT;
    m_io_ring->submit(*this);
}


void WorkItem::submit(const close_callback_func_t& cb)
{
    m_callback = cb;
    m_work_type = Type::CLOSE;
    m_io_ring->submit(*this);
}

SocketType get_type(const AcceptResult& res)
{
    if (res.m_address.get_ipv4())
    {
        return SocketType::IPV4_TCP;
    }
    if (res.m_address.get_ipv6())
    {
        return SocketType::IPV6_TCP;
    }

    return SocketType::UNKNOWN;
}

SocketPortID get_port(const AcceptResult& res)
{
    if (auto addr = res.m_address.get_ipv4())
    {
        return static_cast<SocketPortID>(addr->sin_port);
    }
    if (auto addr = res.m_address.get_ipv6())
    {
        return static_cast<SocketPortID>(addr->sin6_port);
    }
    return SocketPortID::UNKNOWN;
}

} // namespace network