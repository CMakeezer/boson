#ifndef BOSON_EVENTLOOP_H_
#define BOSON_EVENTLOOP_H_
#pragma once

#include <cstdint>
#include <memory>

namespace boson {

struct event_handler {
  virtual void event(int event_id, void* data) = 0;
  virtual void read(int fd, void* data) = 0;
  virtual void write(int fd, void* data) = 0;
};

/**
 * Platform specifi loop implementation
 *
 * We are using a envelop/letter pattern to hide
 * implementation into platform specific code.
 */
class event_loop_impl;

/**
 * event_loop is the class encapsulating the asynchronous I/O loop
 *
 * Since implementation of this loop is platform dependent
 * it must be outside other classes to ease platform specific
 * code integration.
 *
 */
class event_loop {
  std::unique_ptr<event_loop_impl> loop_impl_;

 public:
  ~event_loop();

  /**
   * Creates an event loop
   *
   * An event loop is supposed to be alone to loop
   * into the current thread. Therefore we can store
   * a handler which will stay the same over the loop
   * lifespan
   *
   */
  event_loop(event_handler& handler);

  /**
   * Registers for a long running event listening
   *
   * Unlike read and writes, resgister_event does not work like
   * a one shot request but will live until unregistered
   */
  int register_event(void* data);

  /**
   * Unregister the event and give back its data
   */
  void* unregister_event(int event_id);

  // request_read(int fd, void* data = nullptr);
  // request_write(int fd, void* data = nullptr);
  //
  void send_event(int event);

  void loop(int max_iter = -1);
};
};

#endif  // BOSON_EVENTLOOP_H_
