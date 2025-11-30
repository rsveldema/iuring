
#include <stdio.h>

#include "mocks.hpp"

TEST(check_mocks, declare_tests) {
    printf("hello world\n");

    network::mocks::WorkItem work_item;
    network::mocks::IOUring mock_ring;
}