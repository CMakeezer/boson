#include "internal/netpoller.h"
#include "io_event_loop_impl.h"

namespace boson {
namespace internal {

netpoller_platform_impl::netpoller_platform_impl(io_event_handler& handler)
    : loop_{new io_event_loop(handler, 0)}
{
}

netpoller_platform_impl::~netpoller_platform_impl() {
}

void netpoller_platform_impl::register_fd(fd_t fd)
{
  loop_->register_fd(fd);
}

void netpoller_platform_impl::unregister(fd_t fd)
{
  loop_->unregister(fd);
}

io_loop_end_reason netpoller_platform_impl::loop(int nb_iter, int timeout_ms) {
  return loop_->loop(nb_iter, timeout_ms);
}

void netpoller_platform_impl::interrupt() {
  loop_->interrupt();
}

}
}
