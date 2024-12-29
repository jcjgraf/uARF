#include "log.h"
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
