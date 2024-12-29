#include "log.h"

// Any log with a level higher than this is shown, regardless of the tag
log_level_t log_system_base_level = LOG_LEVEL_INFO;

// Show any log associated with this tag and a level higher than this
// log_level_t log_system_level = LOG_LEVEL_TRACE;
log_level_t log_system_level = LOG_LEVEL_TRACE;
// log_level_t log_system_level = LOG_LEVEL_ERROR;

log_tag_t log_system_tag = LOG_TAG_TEST;
// log_tag_t log_system_tag = LOG_TAG_ALL;
