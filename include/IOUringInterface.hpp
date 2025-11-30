#pragma once

#include <map>

#include "IPAddress.hpp"
#include "Logger.hpp"
#include "UringDefs.hpp"
#include "WorkPool.hpp"

namespace network
{

class IOUringInterface
{
public:
    virtual ~IOUringInterface() {}

    virtual Error init() = 0;

    virtual std::string get_my_mac_address() = 0;

    virtual Error poll_completion_queues() = 0;

    virtual void submit_connect(const std::shared_ptr<ISocket>& socket,
        const IPAddress& target, connect_callback_func_t handler) = 0;

    /** This accepts new connections from other machines.
     * Note that this requires that the socket is opened with
     *  SocketKind::SERVER_STREAM_SOCKET
     * As only server sockets can accept new connections.
     * We check this by asserting the correct behavior here to safeguard this.
     */
    virtual void submit_accept(const std::shared_ptr<ISocket>& socket,
        accept_callback_func_t handler) = 0;

    virtual void submit_recv(const std::shared_ptr<ISocket>& socket,
        recv_callback_func_t handler) = 0;

    /** The steps for sending a packet:
     *      - This returns a work-item where you can retrieve the SendPacket
     * object from
     *      - Then with that send packet you append your dara
     *      - Then you call submit on the work-item.
     *      - The WorkItem::submit() method then has the callback arg.
     */
    virtual std::shared_ptr<WorkItem> submit_send(
        const std::shared_ptr<ISocket>& socket) = 0;

    /** used to submit the submit_sent returned
     * work item.
     */
    virtual void submit(WorkItem& item) = 0;

    virtual void submit_close(const std::shared_ptr<ISocket>& socket,
        close_callback_func_t handler) = 0;
};


} // namespace network