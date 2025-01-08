#include "log.h"
#include <stdlib.h>

#ifdef LOG_TAG
#undef LOG_TAG
#define LOG_TAG LOG_TAG_TEST
#endif

#define TEST_CASE(name)          static unsigned long __text name()
#define TEST_CASE_ARG(name, arg) static unsigned long __text name(void *arg)
#define TEST_SUITE()             int main(void)
#define RUN_TEST_CASE(name)                                                              \
    LOG_INFO("Run TestCase " STR(name) "\n");                                            \
    name()
#define RUN_TEST_CASE_ARG(name, arg)                                                     \
    LOG_INFO("Run TestCase " STR(name) "\n");                                            \
    name(_ptr(arg))

#define TEST_PASS() return 0
#define TEST_ASSERT(cond)                                                                \
    ({                                                                                   \
        if (!(cond)) {                                                                   \
            LOG_WARNING("%s: Assert at %d failed: %s\n", __func__, __LINE__,             \
                        STR((cond)));                                                    \
            exit(1);                                                                     \
        }                                                                                \
    })

#define INIT_SRAND(var)                                                                  \
    uint32_t var;                                                                        \
    asm("rdrand %0" : "=r"(var));                                                        \
    srand(var)
