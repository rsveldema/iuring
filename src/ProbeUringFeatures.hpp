#pragma once

namespace iuring
{
    class ProbeUringFeatures
{
public:
    UringFeature convert_uring_op_to_feature(int op)
    {
        switch (op)
        {
        case IORING_OP_NOP:
            return UringFeature::IORING_OP_NOP;
        case IORING_OP_READV:
            return UringFeature::IORING_OP_READV;
        case IORING_OP_WRITEV:
            return UringFeature::IORING_OP_WRITEV;
        case IORING_OP_FSYNC:
            return UringFeature::IORING_OP_FSYNC;
        case IORING_OP_READ_FIXED:
            return UringFeature::IORING_OP_READ_FIXED;
        case IORING_OP_WRITE_FIXED:
            return UringFeature::IORING_OP_WRITE_FIXED;
        case IORING_OP_POLL_ADD:
            return UringFeature::IORING_OP_POLL_ADD;
        case IORING_OP_POLL_REMOVE:
            return UringFeature::IORING_OP_POLL_REMOVE;
        case IORING_OP_SYNC_FILE_RANGE:
            return UringFeature::IORING_OP_SYNC_FILE_RANGE;
        case IORING_OP_SENDMSG:
            return UringFeature::IORING_OP_SENDMSG;
        case IORING_OP_RECVMSG:
            return UringFeature::IORING_OP_RECVMSG;
        case IORING_OP_TIMEOUT:
            return UringFeature::IORING_OP_TIMEOUT;
        case IORING_OP_TIMEOUT_REMOVE:
            return UringFeature::IORING_OP_TIMEOUT_REMOVE;
        case IORING_OP_ACCEPT:
            return UringFeature::IORING_OP_ACCEPT;
#if SUPPORT_LISTEN_IN_LIBURING
        case IORING_OP_LISTEN:
            return UringFeature::IORING_OP_LISTEN;
#endif
            case IORING_OP_ASYNC_CANCEL:
            return UringFeature::IORING_OP_ASYNC_CANCEL;
        case IORING_OP_LINK_TIMEOUT:
            return UringFeature::IORING_OP_LINK_TIMEOUT;
        case IORING_OP_CONNECT:
            return UringFeature::IORING_OP_CONNECT;
        case IORING_OP_FALLOCATE:
            return UringFeature::IORING_OP_FALLOCATE;
        case IORING_OP_OPENAT:
            return UringFeature::IORING_OP_OPENAT;
        case IORING_OP_CLOSE:
            return UringFeature::IORING_OP_CLOSE;
        case IORING_OP_FILES_UPDATE:
            return UringFeature::IORING_OP_FILES_UPDATE;
        case IORING_OP_STATX:
            return UringFeature::IORING_OP_STATX;
        case IORING_OP_READ:
            return UringFeature::IORING_OP_READ;
        case IORING_OP_WRITE:
            return UringFeature::IORING_OP_WRITE;
        case IORING_OP_FADVISE:
            return UringFeature::IORING_OP_FADVISE;
        case IORING_OP_MADVISE:
            return UringFeature::IORING_OP_MADVISE;
        case IORING_OP_SEND:
            return UringFeature::IORING_OP_SEND;
        case IORING_OP_RECV:
            return UringFeature::IORING_OP_RECV;
        case IORING_OP_OPENAT2:
            return UringFeature::IORING_OP_OPENAT2;
        case IORING_OP_EPOLL_CTL:
            return UringFeature::IORING_OP_EPOLL_CTL;
        case IORING_OP_SPLICE:
            return UringFeature::IORING_OP_SPLICE;
        case IORING_OP_PROVIDE_BUFFERS:
            return UringFeature::IORING_OP_PROVIDE_BUFFERS;
        case IORING_OP_REMOVE_BUFFERS:
            return UringFeature::IORING_OP_REMOVE_BUFFERS;
        case IORING_OP_TEE:
            return UringFeature::IORING_OP_TEE;
        case IORING_OP_SHUTDOWN:
            return UringFeature::IORING_OP_SHUTDOWN;
        case IORING_OP_RENAMEAT:
            return UringFeature::IORING_OP_RENAMEAT;
        case IORING_OP_UNLINKAT:
            return UringFeature::IORING_OP_UNLINKAT;
        case IORING_OP_MKDIRAT:
            return UringFeature::IORING_OP_MKDIRAT;
        case IORING_OP_SYMLINKAT:
            return UringFeature::IORING_OP_SYMLINKAT;
        case IORING_OP_LINKAT:
            return UringFeature::IORING_OP_LINKAT;
        }
        return UringFeature::UNKNOWN;
    }

    ProbeUringFeatures(
        io_uring* ring, logging::ILogger& logger)
        : m_logger(logger)
    {
        m_probe = io_uring_get_probe_ring(ring);
        if (! m_probe)
        {
            LOG_ERROR(m_logger, "failed to probe uring features\n");
            abort();
        }

        for (size_t i = 0; i < m_probe->ops_len; i++)
        {
            const auto op = m_probe->ops[i].op;
            const auto flags = m_probe->ops[i].flags;

            if (!(flags & IO_URING_OP_SUPPORTED))
            {
                continue;
            }

            // fprintf(stderr, "supported: op: {}\n", op);
            const auto ec = convert_uring_op_to_feature(op);
            m_features[ec] = true;
        }
    }

    bool supports(UringFeature f) const
    {
        return m_features.contains(f);
    }

    ~ProbeUringFeatures()
    {
        io_uring_free_probe(m_probe);
    }

private:
    std::map<UringFeature, bool> m_features;
    logging::ILogger& m_logger;
    io_uring_probe* m_probe;
};

}