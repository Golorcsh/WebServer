//
// Created by chenshihao on 2021/12/4.
//

#ifndef WEBSERVER_SRC_LOG_BLOCKDEQUE_H_
#define WEBSERVER_SRC_LOG_BLOCKDEQUE_H_
#include <mutex>
#include <deque>
#include <condition_variable>
#include <ctime>
#include <cassert>

template<typename T>
class BlockDeque {
 public:
  explicit BlockDeque(size_t max_capacity = 100);
  ~BlockDeque();
  void Clear();
  bool Empty();
  bool Full();
  void Close();
  size_t Size();
  size_t Capacity();
  T Front();
  T Back();
  void PushBack(const T &item);
  void PushFront(const T &item);
  bool Pop(T &item);
  bool Pop(T &item, int timeout);
  void Flush();
 private:
  std::deque<T> deque_;
  size_t capacity_;
  std::mutex mutex_;
  bool is_close_;
  std::condition_variable cond_consumer;
  std::condition_variable cond_product;
};
template<typename T>
BlockDeque<T>::BlockDeque(size_t max_capacity):capacity_(max_capacity) {
  assert(max_capacity > 0);
  is_close_ = false;
}
template<typename T>
BlockDeque<T>::~BlockDeque() {
  Close();
}
template<typename T>
void BlockDeque<T>::Close() {
  /*上锁，清空队列，并设置is_close为true*/
  {
    std::lock_guard<std::mutex> locker(mutex_);
    deque_.clear();
    is_close_ = true;
  }
  /*发送通知*/
  cond_product.notify_all();
  cond_consumer.notify_all();
}
template<typename T>
void BlockDeque<T>::Flush() {
  cond_consumer.notify_one();
}
template<typename T>
void BlockDeque<T>::Clear() {
  std::lock_guard<std::mutex> locker(mutex_);
  deque_.clear();
}
template<typename T>
T BlockDeque<T>::Front() {
  std::lock_guard<std::mutex> locker(mutex_);
  return deque_.front();
}
template<typename T>
T BlockDeque<T>::Back() {
  std::lock_guard<std::mutex> locker(mutex_);
  return deque_.back();
}
template<typename T>
size_t BlockDeque<T>::Size() {
  std::lock_guard<std::mutex> locker(mutex_);
  return deque_.size();
}
template<typename T>
size_t BlockDeque<T>::Capacity() {
  return capacity_;
}
template<typename T>
void BlockDeque<T>::PushBack(const T &item) {
  /*上锁*/
  std::unique_lock<std::mutex> locker(mutex_);
  /*当队列大小超出设置的最大容量是，阻塞*/
  while (deque_.size() >= capacity_) {
    cond_product.wait(locker);
  }
  deque_.push_back(item);
  /*通知消费者队列中有数据，可以取*/
  cond_consumer.notify_one();
}
template<typename T>
void BlockDeque<T>::PushFront(const T &item) {
  /*上锁*/
  std::unique_lock<std::mutex> locker(mutex_);
  /*当队列大小超出设置的最大容量是，阻塞*/
  while (deque_.size() >= capacity_) {
    cond_product.wait(locker);
  }
  deque_.push_front(item);
  /*通知消费者队列中有数据，可以取*/
  cond_consumer.notify_one();
}
template<typename T>
bool BlockDeque<T>::Empty() {
  std::lock_guard<std::mutex> locker(mutex_);
  return deque_.empty();
}
template<typename T>
bool BlockDeque<T>::Full() {
  std::lock_guard<std::mutex> locker(mutex_);
  return deque_.size() >= capacity_;
}
template<typename T>
bool BlockDeque<T>::Pop(T &item) {
  /*当队列为空的时候阻塞，直到被通知有数据入队*/
  std::unique_lock<std::mutex> locker(mutex_);
  while (deque_.empty()) {
    cond_consumer.wait(locker);
    if (is_close_)
      return false;
  }
  item = deque_.front();
  deque_.pop_back();
  /*通知生产者已取走一个有空间，可以继续生产*/
  cond_product.notify_one();
  return true;
}
template<typename T>
bool BlockDeque<T>::Pop(T &item, int timeout) {
  /*Pop函数重载版本，可以设置阻塞事件(单位秒)*/
  std::unique_lock<std::mutex> locker(mutex_);
  while (deque_.empty()) {
    if (cond_consumer.wait_for(locker, std::chrono::seconds(timeout)) ==
        std::cv_status::timeout) {
      return false;
    }
    if (is_close_)
      return false;
  }
  item = deque_.front();
  deque_.pop_back();
  /*通知生产者已取走一个有空间，可以继续生产*/
  cond_product.notify_one();
  return true;
}

#endif //WEBSERVER_SRC_LOG_BLOCKDEQUE_H_
