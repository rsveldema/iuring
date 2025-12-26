#pragma once

#include <cstdint>
#include <string>

#include <iuring/IPAddress.hpp>

namespace iuring
{
class ReceivedMessage
{
public:
    ReceivedMessage(const uint8_t* data, size_t size, const IPAddress& sa)
        : m_data(data)
        , m_size(size)
        , m_source_address(sa)
    {
    }

    std::string to_string() const
    {
        return std::string((const char*) begin(), get_size());
    }

    const uint8_t* begin() const
    {
        return m_data;
    }

    bool is_empty() const
    {
        return get_size() == 0;
    }

    size_t get_size() const
    {
        return m_size;
    }

    const uint8_t* end() const
    {
        return m_data + m_size;
    }


    const IPAddress& get_source_address() const
    {
        return m_source_address;
    }

private:
    const uint8_t* m_data;
    size_t m_size;
    IPAddress m_source_address;
};


} // namespace iuring