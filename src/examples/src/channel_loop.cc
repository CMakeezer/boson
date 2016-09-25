#include "boson/channel.h"
#include <unistd.h>
#include <chrono>
#include <iostream>
#include "boson/boson.h"
#include "boson/mutex.h"
#include "boson/logger.h"

using namespace std::literals;

static constexpr int nb_iter = 1e4;
static constexpr int nb_threads = 8;

int main(int argc, char* argv[]) {
  boson::debug::logger_instance(&std::cout);

  std::vector<int> data;
  {
    // Execute a routine communication through pipes
    boson::mutex mut;
    boson::engine instance(nb_threads);
    for (int i = 0; i < nb_threads; ++i) {
      instance.start([&]() {
        for (int j = 0; j < nb_iter; ++j) {
          mut.lock();
          data.push_back(1);
          mut.unlock();
        }
      });
    }
  }
}
