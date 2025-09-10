#pragma once

#include <lib/libc/stdio.hpp>
#include <drivers/virtio-drivers.h>

enum class LOG_LEVEL {
  DEBUG = 0,
  INFO = 1,
  ERROR = 2
};

#define MAX_STR_LEN 512
template <typename... Args> inline void log(LOG_LEVEL log_level, const char *str, Args... args) {
	char buff[MAX_STR_LEN] = { 0 };
	snprintf(buff, 512, str, args...);
    rs_log(static_cast<int>(log_level), buff);
}

template <typename... Args> constexpr void LOG_ERROR(const char *str, Args... args) {
  log(LOG_LEVEL::ERROR, str, args...);
}

template <typename... Args> constexpr void LOG_INFO(const char *str, Args... args) {
  log(LOG_LEVEL::INFO, str, args...);
}
template <typename... Args> constexpr void LOG_DEBUG(const char *str, Args... args) {
  log(LOG_LEVEL::DEBUG, str, args...);
}


