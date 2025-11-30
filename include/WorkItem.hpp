#pragma once

#include <arpa/inet.h>
#include <cassert>
#include <functional>
#include <memory>
#include <optional>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <sys/types.h>
#include <variant>
#include <vector>

#include <liburing.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <sys/types.h>

#include <ISocket.hpp>
#include <NetworkProtocols.hpp>
#include <ReceivedMessage.hpp>
#include <SendPacket.hpp>
#include <UringDefs.hpp>

namespace network
{

enum class [[nodiscard]] ReceivePostAction{ NONE, RE_SUBMIT };

struct AcceptResult
{
    int m_new_fd;
    IPAddress m_address;
};

struct SendResult
{
    int status;
};

struct ConnectResult
{
    int status;
    IPAddress m_address;
};

struct CloseResult
{
    int status;
};

using recv_callback_func_t =
    std::function<ReceivePostAction(const ReceivedMessage& msg)>;
using send_callback_func_t = std::function<void(const SendResult&)>;
using accept_callback_func_t =
    std::function<void(const AcceptResult& new_conn)>;
using connect_callback_func_t =
    std::function<void(const ConnectResult& result)>;
using close_callback_func_t = std::function<void(const CloseResult& result)>;

using work_item_id_t = uint64_t;

class IOUringInterface;


class WorkItem
{
public:
    enum class Type
    {
        UNKNOWN,
        ACCEPT,
        SEND,
        RECV,
        CONNECT,
        CLOSE
    };


    WorkItem(Logger& logger, const std::shared_ptr<IOUringInterface>& network,
        work_item_id_t id, const char* descr, const std::shared_ptr<ISocket>& s)
        : m_logger(logger)
        ,  m_work_type(Type::UNKNOWN)
        , m_io_ring(network)
        , m_id(id)
        , m_socket(s)
        , m_descr(descr)
    {
    }

    bool is_free() const
    {
        return m_state == State::FREE;
    }

    void mark_in_use()
    {
        assert(m_state == State::FREE);
        m_state = State::IN_USE;
    }

    void mark_is_free()
    {
        assert(m_state == State::IN_USE);
        m_state = State::FREE;
    }


    Type get_type() const
    {
        return m_work_type;
    }

    static const char* type_to_string(Type t);

    const char* get_type_str() const
    {
        return type_to_string(get_type());
    }

    bool is_stream() const
    {
        return m_socket->is_stream();
    }

    /** submit a connect request */
    void submit(const IPAddress& target, const connect_callback_func_t& cb);
    /** submit a send request */
    void submit(const send_callback_func_t& cb);
    /** submit a recv request */
    void submit(const recv_callback_func_t& cb);
    /** submit a accept request */
    void submit(const accept_callback_func_t& cb);
    /** submit a close request */
    void submit(const close_callback_func_t& cb);

    void clean_send_packet()
    {
        m_send_packet.reset();
    }

    void examine_control_data();

    void do_normal_socket_event();

    std::shared_ptr<ISocket> get_socket() const
    {
        return m_socket;
    }

    void set_socket(const std::shared_ptr<ISocket>& s, const char* descr)
    {
        m_socket = s;
        m_descr = descr;
    }

    work_item_id_t get_id() const
    {
        return m_id;
    }

    const msghdr& get_msg() const
    {
        return m_msg;
    }

    IPAddress get_sock_addr() const
    {
        return m_sa;
    }

    void call_send_callback(int status)
    {
        assert(std::holds_alternative<send_callback_func_t>(m_callback));
        auto call = std::get<send_callback_func_t>(m_callback);
        SendResult result { status };
        call(result);
        m_send_packet.reset();
    }

    void call_close_callback(int status)
    {
        assert(std::holds_alternative<close_callback_func_t>(m_callback));
        auto call = std::get<close_callback_func_t>(m_callback);

        CloseResult result { status };
        call(result);
    }

    [[nodiscard]] ReceivePostAction call_recv_callback(
        const ReceivedMessage& payload) const
    {
        assert(std::holds_alternative<recv_callback_func_t>(m_callback));
        auto call = std::get<recv_callback_func_t>(m_callback);
        return call(payload);
    }

    void call_accept_callback(const AcceptResult& new_conn) const
    {
        assert(std::holds_alternative<accept_callback_func_t>(m_callback));
        auto call = std::get<accept_callback_func_t>(m_callback);
        call(new_conn);
    }

    void call_connect_callback(const ConnectResult& new_conn) const
    {
        assert(std::holds_alternative<connect_callback_func_t>(m_callback));
        auto call = std::get<connect_callback_func_t>(m_callback);
        call(new_conn);
    }


    void init_send_msg(
        const IPAddress& sock_addr, dscp_t dscp, timetolive_t ttl);

    SendPacket& get_send_packet()
    {
        m_send_packet.reset();
        return m_send_packet;
    }

    const SendPacket& get_raw_send_packet() const
    {
        return m_send_packet;
    }

    bool is_recv_request() const
    {
        return m_work_type == Type::RECV;
    }

    const std::string& get_descr() const
    {
        return m_descr;
    }

    bool next_request_should_wait_for_this_request() const
    {
        return m_link_to_next_request;
    }


private:
    enum class State
    {
        IN_USE,
        FREE
    };

    Logger& m_logger;

    State m_state = State::IN_USE;

    SendPacket m_send_packet;
    Type m_work_type;
    std::shared_ptr<IOUringInterface> m_io_ring;
    work_item_id_t m_id;

    std::variant<connect_callback_func_t, accept_callback_func_t,
        recv_callback_func_t, send_callback_func_t, close_callback_func_t>
        m_callback;

    // used/set when creating submit entry:
    std::array<char, 1024> m_control;
    std::shared_ptr<ISocket> m_socket;
    msghdr m_msg;
    std::shared_ptr<std::array<iovec, 1>> m_msg_iov =
        std::make_shared<std::array<iovec, 1>>();
    IPAddress m_sa;
    sockaddr_storage m_buffer_for_uring;
    socklen_t m_accept_sock_len = 0;
    socklen_t m_connect_sock_len = 0;
    std::string m_descr;

    // if the next request should wait for this one to finish
    bool m_link_to_next_request = false;

    const IPAddress& get_socket_address() const
    {
        return m_sa;
    }

    Logger& get_logger()
    {
        return m_logger;
    }

    ReceivePostAction do_stream_socket_receive();
    ReceivePostAction do_packet_socket_receive();

    friend class IOUring;
};

SocketType get_type(const AcceptResult& res);
SocketPortID get_port(const AcceptResult& res);

} // namespace network