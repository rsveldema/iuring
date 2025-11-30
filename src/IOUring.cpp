#include <ifaddrs.h>
#include <sys/mman.h>

#include <IOUring.hpp>
#include <ProbeUringFeatures.hpp>
#include <thread>

#include <SocketImpl.hpp>
#include <WorkItem.hpp>

namespace network
{
std::shared_ptr<IOUring> IOUring::create(
    Logger& logger, NetworkAdapter& adapter, size_t queue_size)
{
    /** make_shared<> does not work with private ctors
     * so we inherit from it with a public ctor
     * which can be make_shared<>.
     */
    class EnableShared : public IOUring
    {
    public:
        EnableShared(Logger& logger, NetworkAdapter& adapter, size_t queue_size)
            : IOUring(logger, adapter, queue_size)
        {
        }
    };

    return std::make_shared<EnableShared>(logger, adapter, queue_size);
}


IOUring::IOUring(Logger& logger, NetworkAdapter& adapter, size_t queue_size)
    : m_logger(logger)
    , m_queue_size(queue_size)
    , m_adapter(adapter)
    , m_pool(logger)
{
}

IOUring::~IOUring()
{
    io_uring_queue_exit(&m_ring);
}


Error IOUring::init()
{
    init_ring();

    probe_features();

    auto ret = setup_buffer_pool();
    m_initialized = true;
    return ret;
}

void IOUring::init_ring()
{
    if (false)
    {
        struct io_uring_params params;
        memset(&params, 0, sizeof(params));
        params.cq_entries = QD * 8;
        params.flags =
            // IORING_SETUP_IOPOLL | // only for storage
            IORING_SETUP_SUBMIT_ALL |    //
            IORING_SETUP_COOP_TASKRUN |  //
            IORING_SETUP_SINGLE_ISSUER | //
            IORING_SETUP_DEFER_TASKRUN | //
            IORING_SETUP_CQSIZE |        //
            0;

        if (const auto ret = io_uring_queue_init_params(QD, &m_ring, &params);
            ret < 0)
        {
            LOG_ERROR(get_logger(),
                "queue_init failed: %s\n"
                "NB: This requires a kernel version >= 6.0",
                strerror(-ret));
            abort();
        }

        if (1)
        {
            auto ret = io_uring_register_ring_fd(&m_ring);
            if (ret < 0)
            {
                LOG_ERROR(get_logger(), "register_ring_fd: %s", strerror(-ret));
                abort();
            }

            ret = io_uring_close_ring_fd(&m_ring);
            if (ret < 0)
            {
                LOG_ERROR(get_logger(), "close_ring_fd: %s\n", strerror(-ret));
                abort();
            }
        }
    }
    else
    {
        if (const auto ret = io_uring_queue_init(m_queue_size, &m_ring, 0);
            ret != 0)
        {
            LOG_ERROR(
                get_logger(), "io_uring_queue_init: %s\n", strerror(-ret));
            abort();
        }
    }

    io_uring_ring_dontfork(&m_ring);
}

Error IOUring::setup_buffer_pool()
{
    buf_ring_size = (sizeof(io_uring_buf) + buffer_size()) * BUFFERS;
    void* mapped = mmap(NULL, buf_ring_size, PROT_READ | PROT_WRITE,
        MAP_ANONYMOUS | MAP_PRIVATE, 0, 0);
    if (mapped == MAP_FAILED)
    {
        LOG_ERROR(get_logger(), "buf_ring mmap: %s\n", strerror(errno));
        return Error::MMAP_FAILED;
    }
    buf_ring = (struct io_uring_buf_ring*) mapped;

    io_uring_buf_ring_init(buf_ring);

    memset(&m_reg, 0, sizeof(m_reg));
    m_reg.ring_addr = (unsigned long) buf_ring;
    m_reg.ring_entries = BUFFERS;
    m_reg.bgid = 0;
    m_reg.flags = 0;

    buffer_base =
        (unsigned char*) buf_ring + sizeof(struct io_uring_buf) * BUFFERS;

    const auto ret = io_uring_register_buf_ring(&m_ring, &m_reg, 0);
    if (ret)
    {
        LOG_ERROR(get_logger(),
            "buf_ring init failed: %s\n"
            "NB This requires a kernel version >= 6.0\n",
            strerror(-ret));
        return errno_to_error(-ret);
    }

    for (auto i = 0u; i < BUFFERS; i++)
    {
        io_uring_buf_ring_add(buf_ring, get_buffer(i), buffer_size(), i,
            io_uring_buf_ring_mask(BUFFERS), i);
    }
    io_uring_buf_ring_advance(buf_ring, BUFFERS / 2);

    for (int i = BUFFERS / 2; i < BUFFERS; i++)
    {
        m_free_send_ids.push(i);
    }

    return Error::OK;
}

void IOUring::recycle_buffer(int idx)
{
    io_uring_buf_ring_add(buf_ring, get_buffer(idx), buffer_size(), idx,
        io_uring_buf_ring_mask(BUFFERS), 0);
    io_uring_buf_ring_advance(buf_ring, 1);
}


void IOUring::probe_features()
{
    ProbeUringFeatures probe(&m_ring, get_logger());
    assert(probe.supports(UringFeature::IORING_OP_ACCEPT));
    // assert(probe.supports(UringFeature::IORING_OP_LISTEN));
    assert(probe.supports(UringFeature::IORING_OP_RECV));
    assert(probe.supports(UringFeature::IORING_OP_RECVMSG));
    assert(probe.supports(UringFeature::IORING_OP_SEND));
    assert(probe.supports(UringFeature::IORING_OP_SENDMSG));
    assert(probe.supports(UringFeature::IORING_OP_CLOSE));
    assert(probe.supports(UringFeature::IORING_OP_CONNECT));
}


void IOUring::submit_all_requests()
{
    // fprintf(stderr, "SUBMIT REQUEST!\n");
    // unsigned wait_nr = 1;
    // const auto ret = io_uring_submit_and_wait(&m_ring, wait_nr);
    const auto ret = io_uring_submit(&m_ring);
    if (ret < 0)
    {
        fprintf(stderr, "failed to submit sqe: %s\n", strerror(-ret));
    }
    else
    {
        fprintf(stderr, "%d jobs submitted\n", ret);
    }
}


io_uring_sqe* IOUring::get_sqe()
{
    auto* sqe = io_uring_get_sqe(&m_ring);
    if (!sqe)
    {
        LOG_ERROR(get_logger(), "sqe entry is NULL from get-sqe\n");
        submit_all_requests();
        sqe = io_uring_get_sqe(&m_ring);
    }

    if (!sqe)
    {
        LOG_ERROR(get_logger(), "sqe entry is NULL from get-sqe\n");
        abort();
    }
    return sqe;
}


void IOUring::submit(IWorkItem& _item)
{
    auto& item = dynamic_cast<WorkItem&>(_item);
    auto* sqe = get_sqe();
    io_uring_sqe_set_data(sqe, (void*) item.m_id);

    switch (item.get_type())
    {
    default:
        LOG_ERROR(get_logger(), "INTERNAL ERROR: unhandled work item type\n");
        abort();

    case WorkItem::Type::CLOSE:
        io_uring_prep_close(sqe, item.get_socket()->get_fd());
        break;

    case WorkItem::Type::ACCEPT: {
        int flags = 0;
        // flags |= IOSQE_BUFFER_SELECT;

        LOG_INFO(
            get_logger(), "accept on socket %d", item.get_socket()->get_fd());

        item.m_accept_sock_len = 0;
        io_uring_prep_accept(sqe, item.get_socket()->get_fd(),
            (struct sockaddr*) &item.m_buffer_for_uring,
            &item.m_accept_sock_len, flags);
        break;
    }

    case WorkItem::Type::CONNECT: {
        assert(item.m_connect_sock_len > 0);
        const auto fd = item.get_socket()->get_fd();

        sockaddr_in* sa = (sockaddr_in*) &item.m_buffer_for_uring;

        assert(item.m_connect_sock_len == sizeof(*sa));

        fprintf(
            stderr, "prep-connect: %d (port %d)\n", fd, htons(sa->sin_port));

        io_uring_prep_connect(sqe, fd,
            (struct sockaddr*) &item.m_buffer_for_uring,
            item.m_connect_sock_len);
        if (item.next_request_should_wait_for_this_request())
        {
            sqe->flags |= IOSQE_IO_LINK;
        }
        break;
    }

    case WorkItem::Type::RECV: {
        if (item.is_stream())
        {
            int flags = 0;
            fprintf(stderr, " register rcv: %d\n", item.get_socket()->get_fd());
            io_uring_prep_recv(sqe, item.get_socket()->get_fd(),
                nullptr, // buffer selected automatically from buffer queue
                buffer_size(), flags);
        }
        else
        {
            // fprintf(stderr, "RECV ---- submit: %d\n", idx);
            memset(&item.m_msg, 0, sizeof(item.m_msg));


            item.m_msg.msg_name = &item.m_buffer_for_uring;
            item.m_msg.msg_namelen = sizeof(item.m_buffer_for_uring);
            item.m_msg.msg_iov = item.m_msg_iov->data();
            item.m_msg.msg_iovlen = item.m_msg_iov->size();

            assert(item.m_msg.msg_iovlen == 1);

            // fprintf(stderr, "msg_name = %p, p = %p\n",  item.m_msg.msg_name,
            // item.m_msg.msg_iov);

            item.m_msg.msg_iov->iov_base =
                nullptr; // selects a buffer automatically from buffer-queue

            io_uring_prep_recvmsg_multishot(
                sqe, item.get_socket()->get_fd(), &item.m_msg, MSG_TRUNC);
        }

        sqe->flags |= IOSQE_BUFFER_SELECT;
        sqe->buf_group = 0;
        break;
    }

    case WorkItem::Type::SEND: {
        const auto fd = item.get_socket()->get_fd();

        if (item.is_stream())
        {
            int flags = 0;
            const auto& sp = item.get_raw_send_packet();

            LOG_INFO(
                get_logger(), "sending %ld bytes (%s)\n", sp.size(), sp.data());
            io_uring_prep_send(sqe, fd, sp.data(), sp.size(), flags);
        }
        else
        {
            int flags = 0;
            LOG_DEBUG(get_logger(), "SEND ---- submit: %d", fd);
            io_uring_prep_sendmsg(sqe, fd, &item.m_msg, flags);

            // sqe->flags |= IOSQE_FIXED_FILE;
            // sqe->flags |= IOSQE_BUFFER_SELECT;
        }

        if (item.next_request_should_wait_for_this_request())
        {
            sqe->flags |= IOSQE_IO_LINK;
        }
        break;
    }
    }

    submit_all_requests();
}

void IOUring::call_close_callback(
    std::shared_ptr<WorkItem> work_item, io_uring_cqe* cqe)
{
    const int status = cqe->res;
    LOG_DEBUG(get_logger(), "=======> CLOSE CALLBACK: %d\n", cqe->res);
    work_item->call_close_callback(status);
}


void IOUring::call_send_callback(
    std::shared_ptr<WorkItem> work_item, io_uring_cqe* cqe)
{
    LOG_DEBUG(get_logger(), "=======> SEND CALLBACK: %d\n", cqe->res);
    if (cqe->res < 0)
    {
        LOG_ERROR(get_logger(), "recv cqe bad res %d", cqe->res);
        if (cqe->res == -EFAULT || cqe->res == -EINVAL)
        {
            LOG_ERROR(
                get_logger(), "NB: This requires a kernel version >= 6.0\n");
        }
        return;
    }

    work_item->call_send_callback(cqe->res);
}

void IOUring::call_connect_callback(
    std::shared_ptr<WorkItem> work_item, io_uring_cqe* cqe)
{
    LOG_DEBUG(get_logger(), "=======> CONNECT CALLBACK: %d", cqe->res);
    if (cqe->res < 0)
    {
        LOG_ERROR(get_logger(), "recv cqe bad res %d (%s)", cqe->res,
            strerror(-cqe->res));
        if (cqe->res == -EFAULT || cqe->res == -EINVAL)
        {
            LOG_ERROR(
                get_logger(), "NB: This requires a kernel version >= 6.0\n");
        }
        return;
    }

    const int status = cqe->res;
    const network::IPAddress addr(
        work_item->m_buffer_for_uring, work_item->m_connect_sock_len);
    const ConnectResult new_conn{ .status = status, .m_address = addr };

    fprintf(stderr, "CONN- XQE - res = %d\n", status);

    work_item->call_connect_callback(new_conn);
}


void IOUring::call_accept_callback(
    std::shared_ptr<WorkItem> work_item, io_uring_cqe* cqe)
{
    // if (!(cqe->flags & IORING_CQE_F_BUFFER) || cqe->res < 0)
    if (cqe->res < 0)
    {
        LOG_ERROR(get_logger(), "recv cqe bad res %d (%s)", cqe->res,
            strerror(-cqe->res));
        if (cqe->res == -EFAULT || cqe->res == -EINVAL)
        {
            LOG_ERROR(
                get_logger(), "NB: This requires a kernel version >= 6.0\n");
        }
        return;
    }

    const int fd = cqe->res;

    fprintf(stderr, " XQE - res = %d\n", fd);

    const network::IPAddress addr(
        work_item->m_buffer_for_uring, work_item->m_accept_sock_len);
    const AcceptResult new_conn{ .m_new_fd = fd, .m_address = addr };

    work_item->call_accept_callback(new_conn);
}


ReceivePostAction IOUring::call_recv_handler_stream(const uint8_t* buffer,
    std::shared_ptr<WorkItem> work_item, io_uring_cqe* cqe)
{
    IPAddress source_addr;
    const auto payload_length = cqe->res;

    LOG_DEBUG(get_logger(), "size = %d\n", (int) payload_length);

    ReceivedMessage payload(buffer, payload_length, source_addr);

    return work_item->call_recv_callback(payload);
}


ReceivePostAction IOUring::call_recv_handler_datagram(const uint8_t* buffer,
    std::shared_ptr<WorkItem> work_item, io_uring_cqe* cqe)
{
    if (!(cqe->flags & IORING_CQE_F_BUFFER))
    {
        LOG_ERROR(get_logger(), "recv cqe bad res %d", cqe->res);
        abort();
        return ReceivePostAction::RE_SUBMIT;
    }

    auto* recv_msg_out =
        io_uring_recvmsg_validate((void*) buffer, cqe->res, &work_item->m_msg);
    if (!recv_msg_out)
    {
        LOG_ERROR(get_logger(), "bad recvmsg - no recv_msg_out\n");

        return ReceivePostAction::RE_SUBMIT;
    }

    if (recv_msg_out->namelen > sizeof(sockaddr_storage))
    {
        LOG_ERROR(get_logger(), "truncated name\n");

        return ReceivePostAction::RE_SUBMIT;
    }

    if (recv_msg_out->flags & MSG_TRUNC)
    {
        const auto r = io_uring_recvmsg_payload_length(
            recv_msg_out, cqe->res, &work_item->m_msg);

        LOG_ERROR(get_logger(), "truncated msg need %u received %u",
            recv_msg_out->payloadlen, r);

        return ReceivePostAction::RE_SUBMIT;
    }

    network::IPAddress source_addr;
    switch (recv_msg_out->namelen)
    {
    case 0:
    case sizeof(sockaddr_in): {
        const auto* tmp = (sockaddr_in*) io_uring_recvmsg_name(recv_msg_out);
        source_addr = IPAddress(*tmp);
        break;
    }

    case sizeof(sockaddr_in6): {
        const auto* tmp = (sockaddr_in6*) io_uring_recvmsg_name(recv_msg_out);
        source_addr = IPAddress(*tmp);
        break;
    }

    default: {
        fprintf(stderr, "namelen = %d\n", recv_msg_out->namelen);
        abort();
    }
    }

    const auto payload_length = io_uring_recvmsg_payload_length(
        recv_msg_out, cqe->res, &work_item->m_msg);

    LOG_DEBUG(get_logger(),
        "io_uring: received %u bytes (namelen = %d) from %s", payload_length,
        work_item->m_msg.msg_namelen,
        source_addr.to_human_readable_string().c_str());


    auto* ptr =
        (uint8_t*) io_uring_recvmsg_payload(recv_msg_out, &work_item->m_msg);
    assert(ptr);

    ReceivedMessage payload(ptr, payload_length, source_addr);

    return work_item->call_recv_callback(payload);
}


ReceivePostAction IOUring::call_recv_callback(
    std::shared_ptr<WorkItem> work_item, io_uring_cqe* cqe)
{
    if (cqe->res < 0)
    {
        LOG_ERROR(get_logger(), "recv cqe bad res %d (%s)", cqe->res,
            strerror(-cqe->res));
        switch (cqe->res)
        {
        case -EFAULT: {
            LOG_ERROR(get_logger(),
                "NB: This requires a kernel version >= 6.0 (EFAULT)");
            break;
        }

        case -EINVAL: {
            LOG_ERROR(get_logger(),
                "NB: This requires a kernel version >= 6.0 (EINVAL)");
            break;
        }
        }
        abort();
        return ReceivePostAction::RE_SUBMIT;
    }

    const auto idx = cqe->flags >> 16;
    auto* buffer = get_buffer(idx);


    ReceivePostAction ret;
    if (work_item->is_stream())
    {
        ret = call_recv_handler_stream(buffer, work_item, cqe);
    }
    else
    {
        ret = call_recv_handler_datagram(buffer, work_item, cqe);
    }
    recycle_buffer(idx);
    return ret;
}


void IOUring::call_callback_and_free_work_item_id(io_uring_cqe* cqe)
{
    const auto recv_status = cqe->res;
    const auto id = (work_item_id_t) io_uring_cqe_get_data(cqe);

    auto work_item = get_pool().get_work_item(id);
    if (!work_item)
    {
        LOG_ERROR(get_logger(),
            "no work item %ld exists anymore (status %d, flags %d, res = %d)",
            id, recv_status, cqe->flags, cqe->res);
        return;
    }
    assert(work_item);

    if (cqe->res == -ENOBUFS)
    {
        LOG_ERROR(get_logger(),
            "uring ---> ENOBUFS buffer??? -- status: %d (%s)", recv_status,
            work_item->get_descr().c_str());
        return;
    }

    if (cqe->flags & IORING_CQE_F_MORE)
    {
        LOG_DEBUG(get_logger(), "NOTE: more completion events to follow (%s)",
            work_item->get_descr().c_str());
        // return;
    }


    switch (work_item->get_type())
    {
    case WorkItem::Type::ACCEPT:
        call_accept_callback(work_item, cqe);
        submit(*work_item);
        break;

    case WorkItem::Type::CLOSE:
        call_close_callback(work_item, cqe);
        break;

    case WorkItem::Type::RECV: {
        auto ret = call_recv_callback(work_item, cqe);
        switch (ret)
        {
        case ReceivePostAction::NONE:
            break;
        case ReceivePostAction::RE_SUBMIT:
            submit(*work_item);
            break;
        }
        break;
    }

    case WorkItem::Type::CONNECT:
        call_connect_callback(work_item, cqe);
        get_pool().free_work_item(id);
        break;

    case WorkItem::Type::SEND:
        call_send_callback(work_item, cqe);
        get_pool().free_work_item(id);
        break;

    default:
        assert(false);
    }
}


void IOUring::send_packet(const std::shared_ptr<WorkItem>& work_item)
{
    submit(*work_item);
}


Error IOUring::poll_completion_queues()
{
    if (false)
    {
        // fprintf(stderr, "waiting for incoming msgs\n");
        const auto ret = io_uring_submit(&m_ring);
        if (ret < 0)
        {
            perror("failed to io-submit");
            return Error::UNKNOWN;
        }
    }

    // no loop around this to get more bandwidth.
    // We are optimizing for latency/reliable execution of this task.

    io_uring_cqe* cqe = nullptr;
    const auto success = io_uring_peek_cqe(&m_ring, &cqe);
    switch (success)
    {
    case 0:
        fprintf(stderr, "peek successful!\n");

        call_callback_and_free_work_item_id(cqe);
        io_uring_cq_advance(&m_ring, 1);
        break;

    case -EAGAIN:
        break;

    default:
        LOG_ERROR(get_logger(), "failed: %s\n", strerror(-success));
        abort();
        break;
    }
    return Error::OK;
}

void IOUring::submit_accept(
    const std::shared_ptr<ISocket>& socket, accept_callback_func_t handler)
{
    assert(socket->get_kind() == SocketKind::SERVER_STREAM_SOCKET);
    assert(m_initialized);
    get_pool().alloc_accept_work_item(
        socket, shared_from_this(), handler, "accept-job");
}


void IOUring::submit_connect(const std::shared_ptr<ISocket>& socket,
    const IPAddress& target, connect_callback_func_t handler)
{
    assert(m_initialized);
    get_pool().alloc_connect_work_item(
        target, socket, shared_from_this(), handler, "connect-job");
}

void IOUring::submit_recv(
    const std::shared_ptr<ISocket>& socket, recv_callback_func_t handler)
{
    assert(m_initialized);
    get_pool().alloc_recv_work_item(
        socket, shared_from_this(), handler, "read-from-socket");
}

std::shared_ptr<IWorkItem> IOUring::submit_send(
    const std::shared_ptr<ISocket>& socket)
{
    assert(m_initialized);
    auto item = get_pool().alloc_send_work_item(
        socket, shared_from_this(), "write-from-socket");
    return item;
}

void IOUring::submit_close(
    const std::shared_ptr<ISocket>& socket, close_callback_func_t handler)
{
    assert(m_initialized);
    get_pool().alloc_close_work_item(
        socket, shared_from_this(), handler, "close-of-socket");
}


} // namespace network