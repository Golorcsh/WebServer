//
// Created by chenshihao on 2021/12/12.
//

#include "epoller.h"
Epoller::Epoller(int maxEvents) : epollFd_(epoll_create(512)), events_(maxEvents) {
  assert(epollFd_ >= 0 && !events_.empty());
}
Epoller::~Epoller() {
  close(epollFd_);
}
//epoll中添加事件
bool Epoller::AddFd(int fd, uint32_t events) const {
  if (fd < 0)return false;
  epoll_event ev = {0};
  ev.data.fd = fd;
  ev.events = events;
  return 0 == epoll_ctl(epollFd_, EPOLL_CTL_ADD, fd, &ev);
}
//epoll中修改事件
bool Epoller::ModFd(int fd, uint32_t events) const {
  if (fd < 0)return false;
  epoll_event ev = {0};
  ev.data.fd = fd;
  ev.events = events;
  return 0 == epoll_ctl(epollFd_, EPOLL_CTL_MOD, fd, &ev);
}
//epoll中删除事件
bool Epoller::DelFd(int fd) const {
  if (fd < 0)return false;
  return 0 == epoll_ctl(epollFd_, EPOLL_CTL_DEL, fd, nullptr);
}
//阻塞epoll
int Epoller::Wait(int timeoutMs) {
  return epoll_wait(epollFd_, &events_[0], static_cast<int>(events_.size()), timeoutMs);
}
//获得事件对应的fd
int Epoller::GetEventFd(size_t i) const {
  assert(i < events_.size() && i >= 0);
  return events_[i].data.fd;
}
//获得事件
uint32_t Epoller::GetEvents(size_t i) const {
  assert(i < events_.size() && i >= 0);
  return events_[i].events;
}