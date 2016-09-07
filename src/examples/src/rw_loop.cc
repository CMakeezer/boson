#include <iostream>
#include <chrono>
#include "boson/engine.h"
#include "boson/system.h"

using namespace std::literals;

int main(int argc, char* argv[]) {
  // Creates some pipes
  int a2b[2];
  ::pipe(a2b);
  int b2a[2];
  ::pipe(b2a);

  // Execute a routine communication through pipes
  boson::engine instance(1);
  instance.start([&]() {
    int ack_result = 0;
    for (int i = 0; i < 10; ++i) {
      // send data
      boson::write(a2b[1], &i, sizeof(int));
      // Wait for ack
      boson::read(b2a[0], &ack_result, sizeof(int));
      // display info
      if (ack_result == i)
        std::cout << "A: ack succeeded." << std::endl;
    }

    ::close(a2b[1]);
    ::close(b2a[0]);
  });
  instance.start([&]() {
    int result = 0;
    while(result < 9) {
      // Get data
      boson::read(a2b[0],&result,sizeof(int));
      // Send ack
      boson::write(b2a[1],&result,sizeof(int));
      // Display info
      std::cout << "B received: " << result << "\n";
    }

    ::close(a2b[0]);
    ::close(b2a[1]);
  });
}
