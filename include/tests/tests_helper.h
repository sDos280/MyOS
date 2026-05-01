#ifndef TESTS_HELPER_H
#define TESTS_HELPER_H

#include "kernel/print.h"

#define TEST_LOG(tag, fmt, ...) printf("[%-5s] " fmt, tag, ##__VA_ARGS__)

#define TEST_LOG_TEST(fmt, ...) TEST_LOG("TEST", fmt, ##__VA_ARGS__)
#define TEST_LOG_STEP(fmt, ...) TEST_LOG("STEP", fmt, ##__VA_ARGS__)
#define TEST_LOG_OK(fmt, ...)   TEST_LOG("OK",   fmt, ##__VA_ARGS__)
#define TEST_LOG_ERR(fmt, ...)  TEST_LOG("ERR",  fmt, ##__VA_ARGS__)
#define TEST_LOG_INFO(fmt, ...)  TEST_LOG("INFO",  fmt, ##__VA_ARGS__)

#endif // TESTS_HELPER_H