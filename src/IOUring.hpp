#pragma once

/**
 * @file IOUring.hpp
 * @brief Defines the IOUring class for asynchronous I/O operations using
 * io_uring.
 */
#ifndef _GNU_SOURCE
#define _GNU_SOURCE /* See feature_test_macros(7) */
#endif

#include <netdb.h>

#include <liburing.h>

#include <expected>
#include <stack>

#include <slogger/Error.hpp>

#include "iuring/IOUringInterface.hpp"
#include "iuring/NetworkAdapter.hpp"

#include "WorkPool.hpp"


static constexpr size_t DEFAULT_QUEUE_SIZE = 64;

namespace iuring
{
class IOUring : public IOUringInterface,
                public std::enable_shared_from_this<IOUring>
{
private:
    IOUring(
        logging::ILogger& logger, NetworkAdapter& adapter, size_t queue_size);

    IOUring(const IOUring&) = delete;
    IOUring& operator=(const IOUring&) = delete;
    IOUring(IOUring&&) = delete;
    IOUring& operator=(IOUring&&) = delete;

public:
    static std::shared_ptr<IOUring> create(logging::ILogger& logger,
        NetworkAdapter& adapter, size_t queue_size = DEFAULT_QUEUE_SIZE);

    ~IOUring();

    error::Error init() override;

    error::Error poll_completion_queues() override;

    std::shared_ptr<IWorkItem> ackuire_send_workitem(
        const std::shared_ptr<ISocket>& socket) override;

    void submit_connect(const std::shared_ptr<ISocket>& socket,
        const IPAddress& target, connect_callback_func_t handler) override;

    void submit_accept(const std::shared_ptr<ISocket>& socket,
        accept_callback_func_t handler) override;

    void submit_recv(const std::shared_ptr<ISocket>& socket,
        recv_callback_func_t handler) override;

    void submit_close(const std::shared_ptr<ISocket>& socket,
        close_callback_func_t handler) override;

    void resolve_hostname(const std::string& hostname,
        const resolve_hostname_callback_func_t& handler) override;

    NetworkAdapter& get_adapter()
    {
        return m_adapter;
    }

private:
    static constexpr auto QD = 64;
    static constexpr auto BUF_SHIFT = 12; /* 4k */
    static constexpr auto CQES = (QD * 16);
    static constexpr auto BUFFERS = CQES;

    bool m_initialized = false;
    logging::ILogger& m_logger;
    size_t m_queue_size = 0;
    io_uring_buf_reg m_reg;

    io_uring m_ring{};
    io_uring_buf_ring* buf_ring = nullptr;
    size_t buf_ring_size = 0;
    size_t buf_shift = BUF_SHIFT;
    unsigned char* buffer_base = nullptr;
    std::stack<int> m_free_send_ids;

    NetworkAdapter& m_adapter;
    WorkPool m_pool;

    class RequestInfo
    {
    public:
        RequestInfo() = delete;
        RequestInfo(const RequestInfo&) = delete;
        RequestInfo& operator=(const RequestInfo&) = delete;
        RequestInfo& operator=(RequestInfo&&) = delete;


        RequestInfo(RequestInfo&& arg)
        {
            hostname = std::move(arg.hostname);
            handlers = std::move(arg.handlers);

            request = arg.request;
            all_requests[0] = request;

            arg.request = nullptr;
            arg.all_requests[0] = nullptr;
        }

        RequestInfo(const std::string& _hostname)
            : hostname(_hostname)
        {
        }

    public:
        std::string hostname;

        // for getaddrinfo_a
        gaicb* request = new gaicb{};
        gaicb* all_requests[1] = { request };

        // requests for this hostname:
        std::vector<IOUring::resolve_hostname_callback_func_t> handlers;
    };

    std::vector<RequestInfo> m_hostname_DNS_requests;


    logging::ILogger& get_logger()
    {
        return m_logger;
    }


    WorkPool& get_pool()
    {
        return m_pool;
    }

    error::Error setup_buffer_pool();
    void probe_features();
    void init_ring();

    void submit_all_requests();

    size_t buffer_size() const
    {
        assert(buf_shift > 0);
        return 1U << buf_shift;
    }

    uint8_t* get_buffer(int idx)
    {
        assert(buf_shift > 0);
        return buffer_base + (idx << buf_shift);
    }


    static void sig_notifier_hostname_resolve(sigval_t sv);
    void trigger_hostname_resolve_callbacks(void* ptr);

    void recycle_buffer(int idx);

    void submit(IWorkItem& item) override;

    void send_packet(const std::shared_ptr<WorkItem>& work_item);

    void call_callback_and_free_work_item_id(io_uring_cqe* cqe);

    io_uring_sqe* get_sqe();

    void call_send_callback(
        std::shared_ptr<WorkItem> work_item, io_uring_cqe* cqe);

    void call_close_callback(
        std::shared_ptr<WorkItem> work_item, io_uring_cqe* cqe);

    void call_connect_callback(
        std::shared_ptr<WorkItem> work_item, io_uring_cqe* cqe);

    ReceivePostAction call_recv_handler_stream(const uint8_t* buffer,
        std::shared_ptr<WorkItem> work_item, io_uring_cqe* cqe);

    ReceivePostAction call_recv_handler_datagram(const uint8_t* buffer,
        std::shared_ptr<WorkItem> work_item, io_uring_cqe* cqe);

    ReceivePostAction call_recv_callback(
        std::shared_ptr<WorkItem> work_item, io_uring_cqe* cqe);

    void call_accept_callback(
        std::shared_ptr<WorkItem> work_item, io_uring_cqe* cqe);
};
} // namespace iuring