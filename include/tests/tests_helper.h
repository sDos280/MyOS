#ifndef TESTS_HELPER_H
#define TESTS_HELPER_H

#include "kernel/print.h"

#define TEST_COLOR_RESET  "\033[0m"
#define TEST_COLOR_RED    "\033[31m"
#define TEST_COLOR_GREEN  "\033[32m"
#define TEST_COLOR_YELLOW "\033[33m"
#define TEST_COLOR_BLUE   "\033[34m"
#define TEST_COLOR_CYAN   "\033[36m"
#define TEST_COLOR_WHITE  "\033[37m"


#define TEST_LOG_TEST(fmt, ...) printf(TEST_COLOR_WHITE  "[TEST ] " TEST_COLOR_RESET fmt, ##__VA_ARGS__)
#define TEST_LOG_STEP(fmt, ...) printf(TEST_COLOR_CYAN   "[STEP ] " TEST_COLOR_RESET fmt, ##__VA_ARGS__)
#define TEST_LOG_OK(fmt, ...)   printf(TEST_COLOR_GREEN  "[OK   ] " TEST_COLOR_RESET fmt, ##__VA_ARGS__)
#define TEST_LOG_ERR(fmt, ...)  printf(TEST_COLOR_RED    "[ERR  ] " TEST_COLOR_RESET fmt, ##__VA_ARGS__)
#define TEST_LOG_INFO(fmt, ...) printf(TEST_COLOR_BLUE   "[INFO ] " TEST_COLOR_RESET fmt, ##__VA_ARGS__)
#define TEST_LOG_WARN(fmt, ...) printf(TEST_COLOR_YELLOW "[WARN ] " TEST_COLOR_RESET fmt, ##__VA_ARGS__)

#endif // TESTS_HELPER_H