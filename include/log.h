#pragma once
#include "compiler.h"
#include <limits.h>

#include <stdio.h>

#define __ANSI_SEQ(n) "\033[" #n "m"

#define LOG_C_RESET        __ANSI_SEQ(0)
#define LOG_C_BLACK        __ANSI_SEQ(30)
#define LOG_C_DARK_RED     __ANSI_SEQ(31)
#define LOG_C_DARK_GREEN   __ANSI_SEQ(32)
#define LOG_C_DARK_YELLOW  __ANSI_SEQ(33)
#define LOG_C_DARK_BLUE    __ANSI_SEQ(34)
#define LOG_C_DARK_PINK    __ANSI_SEQ(35)
#define LOG_C_DARK_CYAN    __ANSI_SEQ(36)
#define LOG_C_GRAY         __ANSI_SEQ(90)
#define LOG_C_LIGHT_RED    __ANSI_SEQ(91)
#define LOG_C_LIGHT_GREEN  __ANSI_SEQ(92)
#define LOG_C_LIGHT_YELLOW __ANSI_SEQ(93)
#define LOG_C_LIGHT_BLUE   __ANSI_SEQ(94)
#define LOG_C_LIGHT_PINK   __ANSI_SEQ(95)
#define LOG_C_LIGHT_CYAN   __ANSI_SEQ(96)
#define LOG_C_LIGHT_GRAY   __ANSI_SEQ(97)

// Holds all logging levels
typedef enum {
    LOG_LEVEL_TRACE = 0,
    LOG_LEVEL_DEBUG = 1,
    LOG_LEVEL_INFO = 2,
    LOG_LEVEL_WARNING = 3,
    LOG_LEVEL_ERROR = 4,
} log_level_t;

// Holds all available logging tags
typedef enum {
    LOG_TAG_NONE = 0,
    LOG_TAG_MEM = BIT(1),
    LOG_TAG_TEST = BIT(1),
    LOG_TAG_JITA = BIT(2),
    LOG_TAG_VSNIP = BIT(3),
    LOG_TAG_PSNIP = BIT(4),
    LOG_TAG_STUB = BIT(5),
    LOG_TAG_FR = BIT(6),
    LOG_TAG_ALL = ULONG_MAX,
} log_tag_t;

// Any log with a level higher than this is shown, regardless of the tag
extern log_level_t log_system_base_level;
// Show any log associated with this tag and a level higher than this
extern log_level_t log_system_level;
extern log_tag_t log_system_tag;

// Default tag, overwrite in each file
// Do not modifie here!
#define LOG_TAG LOG_TAG_NONE

#define LOG_LOG(print_f, level, tag, prefix, color, format, ...)                         \
    ({                                                                                   \
        if (level >= log_system_base_level ||                                            \
            (level >= log_system_level && (tag & log_system_tag) != 0)) {                \
            print_f(color "[%s] %s:%d:%s " format LOG_C_RESET, prefix, __FILE__,         \
                    __LINE__, __func__ __VA_OPT__(, ) __VA_ARGS__);                      \
        }                                                                                \
    })

#define LOG_TRACE(format, ...)                                                           \
    LOG_LOG(printf, LOG_LEVEL_TRACE, LOG_TAG, "TRACE", LOG_C_LIGHT_GREEN,                \
            format __VA_OPT__(, ) __VA_ARGS__)
#define LOG_DEBUG(format, ...)                                                           \
    LOG_LOG(printf, LOG_LEVEL_DEBUG, LOG_TAG, "DEBUG", LOG_C_LIGHT_BLUE,                 \
            format __VA_OPT__(, ) __VA_ARGS__)
#define LOG_INFO(format, ...)                                                            \
    LOG_LOG(printf, LOG_LEVEL_INFO, LOG_TAG, "INFO", LOG_C_LIGHT_PINK,                   \
            format __VA_OPT__(, ) __VA_ARGS__)
#define LOG_WARNING(format, ...)                                                         \
    LOG_LOG(printf, LOG_LEVEL_WARNING, LOG_TAG, "WARNING", LOG_C_DARK_YELLOW,            \
            format __VA_OPT__(, ) __VA_ARGS__)
#define LOG_ERROR(format, ...)                                                           \
    LOG_LOG(printf, LOG_LEVEL_ERROR, LOG_TAG, "ERROR", LOG_C_DARK_RED,                   \
            format __VA_OPT__(, ) __VA_ARGS__)

#define LOG_TRACE_U(format, ...)                                                         \
    LOG_LOG(printf, LOG_LEVEL_TRACE, LOG_TAG, "TRACE", LOG_C_LIGHT_GREEN,                \
            format __VA_OPT__(, ) __VA_ARGS__)
#define LOG_DEBUG_U(format, ...)                                                         \
    LOG_LOG(printf, LOG_LEVEL_DEBUG, LOG_TAG, "DEBUG", LOG_C_LIGHT_BLUE,                 \
            format __VA_OPT__(, ) __VA_ARGS__)
#define LOG_INFO_U(format, ...)                                                          \
    LOG_LOG(printf, LOG_LEVEL_INFO, LOG_TAG, "INFO", LOG_C_LIGHT_PINK,                   \
            format __VA_OPT__(, ) __VA_ARGS__)
#define LOG_WARNING_U(format, ...)                                                       \
    LOG_LOG(printf, LOG_LEVEL_WARNING, LOG_TAG, "WARNING", LOG_C_DARK_YELLOW,            \
            format __VA_OPT__(, ) __VA_ARGS__)
#define LOG_ERROR_U(format, ...)                                                         \
    LOG_LOG(printf, LOG_LEVEL_ERROR, LOG_TAG, "ERROR", LOG_C_DARK_RED,                   \
            format __VA_OPT__(, ) __VA_ARGS__)
#define HERE()                                                                           \
    printf(LOG_C_DARK_RED "%s, %s, %u\n" LOG_C_RESET, __FILE__, __func__, __LINE__)
