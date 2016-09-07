#include "thread.h"
#include "engine.h"
#include "routine.h"
#include "exception.h"
#include <cassert>
#include <chrono>

namespace boson {
namespace context {

// class engine_proxy;

engine_proxy::engine_proxy(engine& parent_engine) : engine_(&parent_engine) {
}

void engine_proxy::set_id() {
  current_thread_id_ = engine_->register_thread_id();
}

// class thread;

void thread::handle_engine_event() {
  thread_command received_command;
  while (engine_queue_.pop(received_command)) {
    switch (received_command.type) {
      case thread_command_type::add_routine:
        scheduled_routines_.emplace_back(
            received_command.data.template get<routine_ptr_t>().release());
        break;
      case thread_command_type::finish:
        status_ = thread_status::finishing;
        break;
    }
  }
  //execute_scheduled_routines();
}

void thread::unregister_all_events() {
  loop_.unregister_event(engine_event_id_);
  loop_.unregister_event(self_event_id_);
}

thread::thread(engine& parent_engine) : engine_proxy_(parent_engine), loop_(*this) {
  engine_event_id_ = loop_.register_event(&engine_event_id_);
  self_event_id_ = loop_.register_event(&self_event_id_);
}

void thread::event(int event_id, void* data) {
  if (event_id == engine_event_id_) {
    handle_engine_event();
  } else if (event_id == self_event_id_) {
    //execute_scheduled_routines();
  }
}

void thread::read(int fd, void* data) {
}

void thread::write(int fd, void* data) {
}

// callaed by engine
bool thread::push_command(thread_command& command) {
  return engine_queue_.push(command);
};

// called by engine
void thread::execute_commands() {
  loop_.send_event(engine_event_id_);
}

void thread::execute_scheduled_routines() {
  // while (!scheduled_routines_.empty()) {
  decltype(scheduled_routines_) next_scheduled_routines;
  std::list<std::tuple<size_t, routine_ptr_t>> new_timed_routines_;
  while(!scheduled_routines_.empty()) {
    // For now; we schedule them in order
    auto& routine = scheduled_routines_.front();
    routine->resume();
    switch (routine->status()) {
      case routine_status::is_new: {
        // later man
      } break;
      case routine_status::running:
          // Not supposed to happen
          assert(false);
          break;
      case routine_status::yielding: {
        // If not finished, then we reschedule it
        next_scheduled_routines.emplace_back(routine.release());
      } break;
      case routine_status::wait_timer: {
        auto target_data = routine->waiting_data().get<routine_time_point>();
        timed_routines_[target_data].emplace_back(routine.release());
      } break;
      case routine_status::wait_sys_read:
      case routine_status::wait_sys_write:
      case routine_status::wait_channel_read:
      case routine_status::wait_channel_write:
      case routine_status::finished:
        // Nothing to do, the routine can die in the next pop
        break;
    };
    scheduled_routines_.pop_front();
  }

  // Yielded routines are immediately scheduled
  scheduled_routines_ = std::move(next_scheduled_routines);

  // Timed routines are added in the timer list
  // This is made in two step to limit the syscall to get the current date
  

  // If finished and no more routines, exit
  bool no_more_routines = scheduled_routines_.empty() && timed_routines_.empty();
  if (no_more_routines && thread_status::finishing == status_) {
    unregister_all_events();
    status_ = thread_status::finished;
  } else {
    // If some routines already are scheduled, then throw an event to force a loop execution
    if (!scheduled_routines_.empty())
      loop_.send_event(self_event_id_);
  }
}

void thread::loop() {
  using namespace std::chrono;
  engine_proxy_.set_id();  // Tells the engine which thread id we got
  while (status_ != thread_status::finished) {
    // Check if we should have a time out
    int timeout_ms = -1;
    auto first_timed_routines = begin(timed_routines_);
    if (!timed_routines_.empty()) {
      // Compute next timeout
      timeout_ms = duration_cast<milliseconds>(first_timed_routines->first -
                    high_resolution_clock::now())
                       .count();
    }
    auto return_code = loop_.loop(1, timeout_ms);
    switch (return_code) {
      case loop_end_reason::max_iter_reached:
        break;
      case loop_end_reason::timed_out:
        // Shcedule routines that timed out
        for (auto& timed_routine : first_timed_routines->second) {
          timed_routine->expected_event_happened();
          scheduled_routines_.emplace_back(timed_routine.release());
        }
        timed_routines_.erase(first_timed_routines);
        break;
      case loop_end_reason::error_occured:
      default:
       throw exception("Boson unknown error");
        return;
    }
    execute_scheduled_routines();
  }
}
}
}  // namespace boson
