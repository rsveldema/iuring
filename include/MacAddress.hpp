#pragma once

namespace network
{
class MacAddress
{
public:
    explicit MacAddress(const std::string& mac)
        : m_value(mac)
    {
    }

    const std::string& to_string() const
    {
        return m_value;
    }

private:
    std::string m_value;
};
} // namespace network