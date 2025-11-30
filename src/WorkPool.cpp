#include <memory>

#include <WorkPool.hpp>
#include <IOUringInterface.hpp>

namespace network
{
std::shared_ptr<WorkItem> WorkPool::get_work_item(work_item_id_t id)
{
    std::lock_guard lock(m_mutex);
    // assert(id >= 0);
    assert(id <= m_work_items.size());
    auto work_item = m_work_items[id];
    if (!work_item)
    {
        return nullptr;
    }

    assert(work_item != nullptr);
    assert(!work_item->is_free());
    return work_item;
}

void WorkPool::free_work_item(work_item_id_t id)
{
    std::lock_guard lock(m_mutex);
    assert(id <= m_work_items.size());
    auto work_item = m_work_items[id];
    assert(work_item != nullptr);

    assert(!work_item->is_free());
    work_item->mark_is_free();

    m_work_items[id] = nullptr;

    m_free_ids.push(id);
}


std::shared_ptr<WorkItem> WorkPool::internal_alloc_work_item(
    const std::shared_ptr<ISocket>& socket,
    const std::shared_ptr<network::IOUringInterface>& network,
    const char* descr)
{
    if (m_free_ids.empty())
    {
        const auto id = m_work_items.size();
        LOG_INFO(get_logger(), "  NEW: id = %ld (%s)", id, descr);

        auto ret = std::make_shared<WorkItem>(get_logger(), network, id, descr, socket);
        assert(!ret->is_free());
        m_work_items.push_back(ret);
        return ret;
    }
    else
    {
        work_item_id_t id = m_free_ids.top();
        m_free_ids.pop();
        LOG_DEBUG(get_logger(),
            "allocating work item from prepped-queue: %ld (%s)", id, descr);

        auto ret = std::make_shared<WorkItem>(get_logger(), network, id, descr, socket);
        assert(!ret->is_free());
        m_work_items[id] = ret;
        return ret;
    }
}

std::shared_ptr<WorkItem> WorkPool::alloc_send_work_item(
    const std::shared_ptr<ISocket>& socket,
    const std::shared_ptr<network::IOUringInterface>& network,
    const char* descr)
{
    std::lock_guard lock(m_mutex);
    auto wi = internal_alloc_work_item(socket, network, descr);
    assert(wi);
    return wi;
}

std::shared_ptr<WorkItem> WorkPool::alloc_recv_work_item(
    const std::shared_ptr<ISocket>& socket,
    const std::shared_ptr<network::IOUringInterface>& network,
    const recv_callback_func_t& callback, const char* descr)
{
    std::lock_guard lock(m_mutex);
    auto wi = internal_alloc_work_item(socket, network, descr);
    assert(wi);
    wi->submit(callback);
    return wi;
}

std::shared_ptr<WorkItem> WorkPool::alloc_accept_work_item(
    const std::shared_ptr<ISocket>& socket,
    const std::shared_ptr<network::IOUringInterface>& network,
    const accept_callback_func_t& callback, const char* descr)
{
    std::lock_guard lock(m_mutex);
    auto wi = internal_alloc_work_item(socket, network, descr);
    assert(wi);
    wi->submit(callback);
    return wi;
}

std::shared_ptr<WorkItem> WorkPool::alloc_connect_work_item(
    const IPAddress& target,
    const std::shared_ptr<ISocket>& socket,
    const std::shared_ptr<network::IOUringInterface>& network,
    const connect_callback_func_t& callback, const char* descr)
{
    std::lock_guard lock(m_mutex);
    auto wi = internal_alloc_work_item(socket, network, descr);
    assert(wi);
    wi->submit(target, callback);
    return wi;
}


std::shared_ptr<WorkItem> WorkPool::alloc_close_work_item(
    const std::shared_ptr<ISocket>& socket,
    const std::shared_ptr<network::IOUringInterface>& network,
    const close_callback_func_t& callback, const char* descr)
{
    std::lock_guard lock(m_mutex);
    auto wi = internal_alloc_work_item(socket, network, descr);
    assert(wi);
    wi->submit(callback);
    return wi;
}

} // namespace network