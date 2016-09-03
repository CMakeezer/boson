#include "routine.h"

namespace boson {

/**
 * Store the local thread context
 *
 * The usage of this breaks encapsulation sinces routines are not supposed
 * to know about what is runniung them. But it will avoid too much load/store
 * from a thread_local variable since it implies a look in a map
 */
thread_local context::transfer_t current_thread_context = { nullptr, nullptr };

namespace detail {
void resume_routine(context::transfer_t transfered_context) {
  routine_context rcontext{transfered_context};
  auto current_routine = static_cast<routine*>(transfered_context.data);
  context::transfer_t& main_context = current_thread_context;
  main_context = transfered_context;
  //(*current_routine->func_)(rcontext);
  current_routine->status_ = routine_status::running;
  (*current_routine->func_)();
  current_routine->status_ = routine_status::finished;
  // It is paramount to use reference to the thread local variable here
  // since it can be updated during ping/ping context jumps during the routine
  // execution
  context::jump_fcontext(main_context.fctx, main_context.data);
}
}

void yield() {
  context::transfer_t& main_context = current_thread_context;
  //context::jump_fcontext(main_context.fctx,nullptr);
  routine* current_routine = static_cast<routine*>(main_context.data);
  current_routine->status_ = routine_status::yielding;
  main_context = context::jump_fcontext(main_context.fctx,nullptr);
}
}  // namespace boson
