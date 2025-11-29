#pragma once

#include <stack>

#include "Error.hpp"
#include "IOUringInterface.hpp"

#include <liburing.h>

static constexpr size_t DEFAULT_QUEUE_SIZE = 64;

namespace network
{
class IOUring : public IOUringInterface, public std::enable_shared_from_this<IOUring>
{
public:
    IOUring(Logger& logger, const std::string& interface_name, bool tune, size_t queue_size = DEFAULT_QUEUE_SIZE);
    ~IOUring();

    IOUring(const IOUring&) = delete;
    IOUring& operator=(const IOUring&) = delete;
    IOUring(IOUring&&) = delete;
    IOUring& operator=(IOUring&&) = delete;

    Error init() override;
    Error poll_completion_queues() override;

    void submit_connect(const std::shared_ptr<ISocket>& socket, const IPAddress& target) override;
    void submit_all_requests() override;

private:
    static constexpr auto QD = 64;
    static constexpr auto BUF_SHIFT = 12; /* 4k */
    static constexpr auto CQES = (QD * 16);
    static constexpr auto BUFFERS = CQES;

    size_t m_queue_size;

    io_uring m_ring{};
    io_uring_buf_ring* buf_ring = nullptr;
    size_t buf_ring_size = 0;
    size_t buf_shift = BUF_SHIFT;
    unsigned char* buffer_base = nullptr;
    std::stack<int> m_free_send_ids;


    Error setup_buffer_pool();
    void probe_features();

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

    void recycle_buffer(int idx);

    void re_submit(WorkItem& item) override;

    void send_packet(const std::shared_ptr<WorkItem>& work_item);

    void call_callback_and_free_work_item_id(io_uring_cqe* cqe);

    io_uring_sqe* get_sqe();

    void call_send_callback(
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
} // namespace network