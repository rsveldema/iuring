#pragma once

#include <functional>
#include <string>

/**
 * @file CompletionCallbacks.hpp
 * @brief Defines callback function types for network operations.
 */

namespace iuring
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

class ReceivedMessage;

using recv_callback_func_t =
    std::function<ReceivePostAction(const ReceivedMessage& msg)>;

using send_callback_func_t = std::function<void(const SendResult&)>;

using accept_callback_func_t =
    std::function<void(const AcceptResult& new_conn)>;

using connect_callback_func_t =
    std::function<void(const ConnectResult& result)>;

using close_callback_func_t = std::function<void(const CloseResult& result)>;

} // namespace iuring