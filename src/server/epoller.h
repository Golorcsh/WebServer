//
// Created by chenshihao on 2021/12/12.
//
/*对epoll进行封装*/

#ifndef WEBSERVER_SRC_SERVER_EPOLLER_H_
#define WEBSERVER_SRC_SERVER_EPOLLER_H_

#include <sys/epoll.h> //epoll_ctl()
#include <fcntl.h>  // fcntl()
#include <unistd.h> // close()
#include <assert.h> // close()
#include <vector>
#include <errno.h>

class Epoller {
 public:
    explicit Epoller(int maxEvent = 1024);

  ~Epoller();

    bool AddFd(int fd, uint32_t events);

    bool ModFd(int fd, uint32_t events);

    bool DelFd(int fd);

  int Wait(int timeoutMs = -1);
  int GetEventFd(size_t i) const;
  uint32_t GetEvents(size_t i) const;

 private:
  int epollFd_;
  std::vector<struct epoll_event> events_;
};

#endif //WEBSERVER_SRC_SERVER_EPOLLER_H_
