//
// Created by chenshihao on 2021/12/7.
//

#ifndef WEBSERVER_SRC_HEAPTIMER_H_
#define WEBSERVER_SRC_HEAPTIMER_H_
#include <queue>
#include <unordered_map>
#include <time.h>
#include <algorithm>
#include <arpa/inet.h>
#include <functional>
#include <assert.h>
#include <chrono>
#include "../log/log.h"

using TimeoutCallBack = std::function<void()>;
using Clock = std::chrono::high_resolution_clock;/*拥有最小刻度的时间*/
using MS = std::chrono::microseconds;/*毫秒*/
using TimeStamp = Clock::time_point;
#endif //WEBSERVER_SRC_HEAPTIMER_H_

struct TimerNode {
  int id;
  TimeStamp expires;
  TimeoutCallBack cb;
  bool operator<(const TimerNode &t) const {/*重载比大小*/
    return expires < t.expires;
  }
};

class HeapTimer {
 public:
  HeapTimer() { heap_.reserve(64); }
  ~HeapTimer() { clear(); }
  void adjust(int id, int timeout);
  void add(int id, int timeout, const TimeoutCallBack &cb);
  void doWork(int id);
  void clear();
  void tick();
  void pop();
  int GetNextTick();
 private:
  void del_(size_t i);
  void siftup_(size_t i);
  bool siftdown_(size_t index, size_t n);
  void SwapNode_(size_t i, size_t j);
  /*使用vector做为队列(配合hashtable)而不使用priority_queue*/
  /*这更方便调整*/
  std::vector<TimerNode> heap_;
  /*key为id,value为下标*/
  std::unordered_map<int, size_t> ref_;
};