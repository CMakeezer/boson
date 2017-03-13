#ifndef BOSON_IOEVENTLOOP_H_
#define BOSON_IOEVENTLOOP_H_
#pragma once

#include <cstdint>
#include <memory>
#include <tuple>
#include "system.h"

namespace boson {

//enum class event_status { ok, panic, hang_up };
using event_status = int;

union event_data {
  void* ptr;
  int fd;
  uint32_t u32;
  uint64_t u64;
};

struct io_event_handler {
  virtual void read(fd_t fd, event_status status) = 0;
  virtual void write(fd_t fd, event_status status) = 0;
  virtual void closed(fd_t fd) = 0;
};

enum class io_loop_end_reason { max_iter_reached, timed_out, error_occured };

/**
 * Platform specific loop implementation
 *
 * We are using a envelop/letter pattern to hide
 * implementation into platform specific code.
 */
class io_event_loop;

};

extern template class std::unique_ptr<boson::io_event_loop>;

#endif  // BOSON_IOEVENTLOOP_H_
