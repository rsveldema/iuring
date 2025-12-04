#include <gtest/gtest.h>

#include <iuring/StringUtils.hpp>

namespace Tests
{
TEST(TestStrings, test_endswith)
{
    ASSERT_TRUE(StringUtils::ends_with("", ""));
    ASSERT_TRUE(StringUtils::ends_with("hi", ""));
    ASSERT_TRUE(StringUtils::ends_with("hi", "hi"));
    ASSERT_TRUE(StringUtils::ends_with("khi", "hi"));

    ASSERT_FALSE(StringUtils::ends_with("xhi_", "hi"));
    ASSERT_FALSE(StringUtils::ends_with("hii", "hi"));
}

TEST(TestStrings, test_trim)
{
    ASSERT_EQ(StringUtils::trim("  "), " ");
    ASSERT_EQ(StringUtils::trim("hi "), "hi");
    ASSERT_EQ(StringUtils::trim(" hi"), " hi");
    ASSERT_EQ(StringUtils::trim(" hi "), " hi");
}

TEST(TestStrings, test_to_mdns_string)
{
    const char expected_str[] = "\3abc\4bcde";
    std::string expected{ expected_str, sizeof(expected_str) };
    ASSERT_EQ(StringUtils::to_mdns_string({ { "abc" }, { "bcde" } }), expected);
}

TEST(TestStrings, test_to_string_of_vec_of_strings)
{
    ASSERT_EQ(StringUtils::to_string({}), "[]");
    ASSERT_EQ(StringUtils::to_string({ { "" } }), "[]");
    ASSERT_EQ(StringUtils::to_string({ { "abc" } }), "[abc]");
    ASSERT_EQ(StringUtils::to_string({ { "abc" }, { "bcde" } }), "[abc, bcde]");
}

TEST(TestStrings, test_array_to_string)
{
    std::array<int,3> iarr = {1,2,3};
    ASSERT_EQ(StringUtils::array_to_string(iarr), "1,2,3");

    std::array<double,3> sarr = {1,2,3};
    ASSERT_EQ(StringUtils::array_to_string(sarr),  "1.000000,2.000000,3.000000");
}

TEST(TestStrings, test_parse_int)
{
    ASSERT_EQ(StringUtils::parse_int(""), std::nullopt);
    ASSERT_EQ(StringUtils::parse_int("x"), std::nullopt);
    ASSERT_EQ(StringUtils::parse_int("42"), 42);
}

TEST(TestStrings, test_upper)
{
    ASSERT_EQ(StringUtils::to_upper(""), "");
    ASSERT_EQ(StringUtils::to_upper("_aBc 123"), "_ABC 123");
}

TEST(TestStrings, test_split)
{
    ASSERT_EQ(StringUtils::split("", ','), (std::vector<std::string> {} ));
    ASSERT_EQ(StringUtils::split("abc", ','), (std::vector<std::string> {{"abc"}} ));
    ASSERT_EQ(StringUtils::split("a,b,c", ','), (std::vector<std::string> {{"a"}, {"b"}, {"c"}} ));
}

} // namespace Tests