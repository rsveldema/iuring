#pragma once

#include <optional>
#include <stdint.h>
#include <array>
#include <string>
#include <vector>

/**
 * note: these functions are functional: they return the result and do not
 * change their arguments
 */

namespace StringUtils
{
template <typename T> bool contains(const std::vector<T>& l, const T elt)
{
    for (auto it : l)
    {
        if (it == elt)
        {
            return true;
        }
    }
    return false;
}

/** @returns comma seperated string
 */
template <typename T, size_t N>
std::string array_to_string(const std::array<T, N>& arr)
{
    const char* comma = "";
    std::string ret;
    for (size_t i = 0; i < N; i++)
    {
        ret += comma;
        ret += std::to_string(arr[i]);
        comma = ",";
    }
    return ret;
}


/** Result are appended strings with each prefixed by a byte of length
*/
std::string to_mdns_string(const std::vector<std::string>& list);

/** creates a string: [elt1, elt2, ...] */
std::string to_string(const std::vector<std::string>& list);

[[maybe_unused]]
static inline
const char* to_string(bool v) {
    return v ? "true" : "false";
}


std::vector<std::string> split(const std::string_view& s, const char sep);

bool ends_with(const std::string_view& s, const std::string_view& end);

std::optional<uint32_t> hex_string_to_int(const std::string& s);

std::string trim(const std::string_view& s);

std::optional<int32_t> parse_int(const std::string& value);

std::string replace(const std::string_view& in, char from, char to);

std::string remove(const std::string_view& in, char delete_char);

std::string to_upper(const std::string& s);

} // namespace StringUtils
