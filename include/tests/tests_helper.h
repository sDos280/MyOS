#ifndef TESTS_HELPER_H
#define TESTS_HELPER_H

#include "kernel/print.h"

/* foreground normal */
#define TEST_FG_BLACK         "\033[30m"
#define TEST_FG_RED           "\033[31m"
#define TEST_FG_GREEN         "\033[32m"
#define TEST_FG_BROWN         "\033[33m"
#define TEST_FG_BLUE          "\033[34m"
#define TEST_FG_MAGENTA       "\033[35m"
#define TEST_FG_CYAN          "\033[36m"
#define TEST_FG_LIGHT_GRAY    "\033[37m"

/* background normal */
#define TEST_BG_BLACK         "\033[40m"
#define TEST_BG_RED           "\033[41m"
#define TEST_BG_GREEN         "\033[42m"
#define TEST_BG_BROWN         "\033[43m"
#define TEST_BG_BLUE          "\033[44m"
#define TEST_BG_MAGENTA       "\033[45m"
#define TEST_BG_CYAN          "\033[46m"
#define TEST_BG_LIGHT_GRAY    "\033[47m"

/* light foreground normal */
#define TEST_FG_DARK_GRAY      "\033[90m"
#define TEST_FG_LIGHT_RED      "\033[91m"
#define TEST_FG_LIGHT_GREEN    "\033[92m"
#define TEST_FG_YELLOW         "\033[93m"
#define TEST_FG_LIGHT_BLUE     "\033[94m"
#define TEST_FG_LIGHT_MAGENTA  "\033[95m"
#define TEST_FG_LIGHT_CYAN     "\033[96m"
#define TEST_FG_WHITE          "\033[97m"

/* light background normal */
#define TEST_BG_GRAY           "\033[100m"
#define TEST_BG_LIGHT_RED      "\033[101m"
#define TEST_BG_LIGHT_GREEN    "\033[102m"
#define TEST_BG_LIGHT_YELLOW   "\033[103m"
#define TEST_BG_LIGHT_BLUE     "\033[104m"
#define TEST_BG_LIGHT_MAGENTA  "\033[105m"
#define TEST_BG_LIGHT_CYAN     "\033[106m"
#define TEST_BG_WHITE          "\033[107m"


#define TEST_COLOR_RESET "\033[0m"

#define TEST_LOG_TEST(fmt, ...) printf(TEST_FG_LIGHT_MAGENTA "[TEST ] " TEST_COLOR_RESET fmt, ##__VA_ARGS__)
#define TEST_LOG_STEP(fmt, ...) printf(TEST_FG_CYAN          "[STEP ] " TEST_COLOR_RESET fmt, ##__VA_ARGS__)
#define TEST_LOG_OK(fmt, ...)   printf(TEST_FG_GREEN         "[OK   ] " TEST_COLOR_RESET fmt, ##__VA_ARGS__)
#define TEST_LOG_ERR(fmt, ...)  printf(TEST_FG_RED           "[ERR  ] " TEST_COLOR_RESET fmt, ##__VA_ARGS__)
#define TEST_LOG_WARN(fmt, ...) printf(TEST_FG_YELLOW        "[WARN ] " TEST_COLOR_RESET fmt, ##__VA_ARGS__)
#define TEST_LOG_INFO(fmt, ...) printf(TEST_FG_BLUE          "[INFO ] " TEST_COLOR_RESET fmt, ##__VA_ARGS__)

#endif // TESTS_HELPER_H