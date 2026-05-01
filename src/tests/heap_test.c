#include "tests/test_log.h"
#include "mm/kheap.h"
#include "utils.h"

static int heap_check_pattern(uint8_t *buf, size_t size, uint8_t value)
{
    for (size_t i = 0; i < size; i++) {
        if (buf[i] != value) {
            TEST_LOG_ERR("Pattern mismatch at byte %u expected=0x%x got=0x%x\n",
                         i, value, buf[i]);
            return 0;
        }
    }

    return 1;
}

void heap_test_basic(void)
{
    TEST_LOG_TEST("Heap basic test start\n");

    TEST_LOG_STEP("Allocating 64 bytes\n");
    uint8_t *a = (uint8_t *)kalloc(64);
    if (!a) {
        TEST_LOG_ERR("kalloc(64) returned NULL\n");
        return;
    }
    TEST_LOG_OK("Allocated block A=%x\n", a);

    TEST_LOG_STEP("Allocating 128 bytes\n");
    uint8_t *b = (uint8_t *)kalloc(128);
    if (!b) {
        TEST_LOG_ERR("kalloc(128) returned NULL\n");
        kfree(a);
        return;
    }
    TEST_LOG_OK("Allocated block B=%x\n", b);

    TEST_LOG_STEP("Allocating 256 bytes\n");
    uint8_t *c = (uint8_t *)kalloc(256);
    if (!c) {
        TEST_LOG_ERR("kalloc(256) returned NULL\n");
        kfree(b);
        kfree(a);
        return;
    }
    TEST_LOG_OK("Allocated block C=%x\n", c);

    TEST_LOG_STEP("Writing patterns\n");
    memset(a, 0xAA, 64);
    memset(b, 0xBB, 128);
    memset(c, 0xCC, 256);

    TEST_LOG_STEP("Verifying patterns\n");
    if (!heap_check_pattern(a, 64, 0xAA)) return;
    if (!heap_check_pattern(b, 128, 0xBB)) return;
    if (!heap_check_pattern(c, 256, 0xCC)) return;
    TEST_LOG_OK("Patterns verified\n");

    TEST_LOG_STEP("Freeing middle block B\n");
    kfree(b);
    TEST_LOG_OK("Freed block B\n");

    TEST_LOG_STEP("Allocating 100 bytes, should reuse/split freed space if supported\n");
    uint8_t *d = (uint8_t *)kalloc(100);
    if (!d) {
        TEST_LOG_ERR("kalloc(100) returned NULL\n");
        kfree(c);
        kfree(a);
        return;
    }
    TEST_LOG_OK("Allocated block D=%x\n", d);

    TEST_LOG_STEP("Writing and verifying block D\n");
    memset(d, 0xDD, 100);
    if (!heap_check_pattern(d, 100, 0xDD)) return;
    TEST_LOG_OK("Block D verified\n");

    TEST_LOG_STEP("Ensuring old blocks A and C were not corrupted\n");
    if (!heap_check_pattern(a, 64, 0xAA)) return;
    if (!heap_check_pattern(c, 256, 0xCC)) return;
    TEST_LOG_OK("A and C still valid\n");

    TEST_LOG_STEP("Freeing all blocks\n");
    kfree(d);
    kfree(c);
    kfree(a);
    TEST_LOG_OK("All blocks freed\n");

    TEST_LOG_TEST("PASS - Heap basic test succeeded\n");
}

void heap_test_many_small_allocs(void)
{
    enum { COUNT = 64, SIZE = 32 };

    uint8_t *ptrs[COUNT];

    TEST_LOG_TEST("Heap many-small-allocs test start\n");

    memset(ptrs, 0, sizeof(ptrs));

    TEST_LOG_STEP("Allocating %u blocks of %u bytes\n", COUNT, SIZE);

    for (uint32_t i = 0; i < COUNT; i++) {
        ptrs[i] = (uint8_t *)kalloc(SIZE);
        if (!ptrs[i]) {
            TEST_LOG_ERR("kalloc failed at index=%u\n", i);
            return;
        }

        memset(ptrs[i], (uint8_t)i, SIZE);
    }

    TEST_LOG_OK("All small blocks allocated\n");

    TEST_LOG_STEP("Verifying all small blocks\n");

    for (uint32_t i = 0; i < COUNT; i++) {
        if (!heap_check_pattern(ptrs[i], SIZE, (uint8_t)i))
            return;
    }

    TEST_LOG_OK("All small blocks verified\n");

    TEST_LOG_STEP("Freeing all small blocks\n");

    for (uint32_t i = 0; i < COUNT; i++) {
        kfree(ptrs[i]);
    }

    TEST_LOG_OK("All small blocks freed\n");

    TEST_LOG_TEST("PASS - Heap many-small-allocs test succeeded\n");
}