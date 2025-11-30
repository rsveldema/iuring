#pragma once

#include <functional>

#include "ReceivedMessage.hpp"
#include "SendPacket.hpp"
#include "CompletionCallbacks.hpp"

namespace network
{
class IWorkItem
{
public:
    virtual ~IWorkItem() {}

    virtual SendPacket& get_send_packet() = 0;
    virtual void submit(const send_callback_func_t& cb) = 0;
};
} // namespace network
