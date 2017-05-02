#include <iostream>
#include <chrono>
#include <set>
#include <string>
#include <atomic>
#include "boson/boson.h"
#include "boson/channel.h"
#include "boson/net/socket.h"
#include "boson/select.h"
#include "boson/shared_buffer.h"
#include "fmt/format.h"
#include "iostream"
#include <fcntl.h>
#include <signal.h>

// sudo perf stat -e 'syscalls:sys_enter_epoll*'

using namespace boson;
using namespace std::chrono_literals;

void listen_client(int fd, channel<std::string, 5> msg_chan) {
  std::array<char, 2048> buffer;
  for (;;) {
    ssize_t nread = boson::read(fd, buffer.data(), buffer.size());
    if (nread <= 1) {
      boson::close(fd);
      return;
    }
    std::string message(buffer.data(), nread - 2);
    if (message == "quit") {
      boson::close(fd);
      return;
    }
    msg_chan << message;
  }
}

void handleNewConnections(boson::channel<int, 5> newConnChan) {
  int sockfd = net::create_listening_socket(8080);
  struct sockaddr_in cli_addr;
  socklen_t clilen = sizeof(cli_addr);
  for (;;) {
    int newFd = boson::accept(sockfd, (struct sockaddr *)&cli_addr, &clilen);
    if (0 <= newFd) {
      newConnChan << newFd;
    }
  }
}

//void displayCounter(uint64_t* counter) {
void displayCounter(std::atomic<uint64_t>* counter) {
  using namespace std::chrono;
  auto latest = high_resolution_clock::now();
  milliseconds step_duration {1000};
  for (;;) {
    ::printf("%lu\n",counter->exchange(0,std::memory_order_relaxed)*1000/step_duration.count());
    //::printf("%lu\n",(*counter)*1000/step_duration.count());
    //*counter = 0;
    //::printf("%lu\n",counter->exchange(0,std::memory_order_relaxed));
    boson::sleep(1000ms);
    auto current = high_resolution_clock::now();
    step_duration = duration_cast<milliseconds>(current - latest);
    latest = current;
  }
}

int main(int argc, char *argv[]) {
  struct sigaction action {SIG_IGN,0,0};
  ::sigaction(SIGPIPE, &action, nullptr);
  boson::run(4, []() {
    channel<int, 5> new_connection;
    channel<std::string, 5> messages;
    std::atomic<uint64_t> counter {0};
    //uint64_t counter {0};
    boson::start(displayCounter, &counter);
    boson::start(handleNewConnections, new_connection);

    // Create socket and list to connections

    // Main loop
    std::set<int> conns;
    for (;;) {
      int conn = 0;
      int scheduler = 0;
      std::string message;
      select_any(                           //
          event_read(new_connection, conn,  //
                     [&](bool) {            //
                       conns.insert(conn);
                       start_explicit(++scheduler % 3 + 1, listen_client, conn, messages);
                       //start(listen_client, conn, messages);
                     }),
          event_read(messages, message,
                     [&](bool) {  //
                       message += "\n";
                       start([&conns, &counter](std::string&& message) {
                         for (auto fd : conns) {
                           boson::write(fd, message.data(), message.size());
                           //++counter;
                           counter.fetch_add(1, std::memory_order_relaxed);
                         }
                       }, std::move(message));
                     }));
    };
  });
}
