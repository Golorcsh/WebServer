//
// Created by chenshihao on 2021/12/4.
//

#ifndef WEBSERVER_SRC_LOG_BLOCKDEQUE_H_
#define WEBSERVER_SRC_LOG_BLOCKDEQUE_H_
#include <mutex>
#include <deque>
#include <condition_variable>
#include <ctime>

template<typename T>
class BlockDeque {
 public:
  explicit BlockDeque(size_t MaxCapacity = 1000);

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
  std::deque<T> deq_;
  size_t capacity_;
  std::mutex mutex_;
  bool is_close_;
  std::condition_variable cond_consumer_;
  std::condition_variable condition_variable_;
};
template<typename T>
BlockDeque<T>::BlockDeque(size_t MaxCapacity):capacity_(MaxCapacity) {
  assert(MaxCapacity > 0);
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
    deq_.clear();
    is_close_ = true;
  }
  /*发送通知*/
  condition_variable_.notify_all();
  cond_consumer_.notify_all();
}
template<typename T>
void BlockDeque<T>::Flush() {
  cond_consumer_.notify_one();
}
template<typename T>
void BlockDeque<T>::Clear() {
  std::lock_guard<std::mutex> locker(mutex_);
  deq_.clear();
}
template<typename T>
T BlockDeque<T>::Front() {
  std::lock_guard<std::mutex> locker(mutex_);
  return deq_.front();
}
template<typename T>
T BlockDeque<T>::Back() {
  std::lock_guard<std::mutex> locker(mutex_);
  return deq_.back();
}
template<typename T>
size_t BlockDeque<T>::Size() {
  std::lock_guard<std::mutex> locker(mutex_);
  return deq_.size();
}
template<typename T>
size_t BlockDeque<T>::Capacity() {
  std::lock_guard<std::mutex> locker(mutex_);
  return capacity_;
}
template<typename T>
void BlockDeque<T>::PushBack(const T &item) {
  /*上锁*/
  std::unique_lock<std::mutex> locker(mutex_);
  /*当队列大小超出设置的最大容量是，阻塞*/
  while (deq_.size() >= capacity_) {
    condition_variable_.wait(locker);
  }
  deq_.push_back(item);
  /*通知消费者队列中有数据，可以取*/
  cond_consumer_.notify_one();
}
template<typename T>
void BlockDeque<T>::PushFront(const T &item) {
  /*上锁*/
  std::unique_lock<std::mutex> locker(mutex_);
  /*当队列大小超出设置的最大容量是，阻塞*/
  while (deq_.size() >= capacity_) {
    condition_variable_.wait(locker);
  }
  deq_.push_front(item);
  /*通知消费者队列中有数据，可以取*/
  cond_consumer_.notify_one();
}
template<typename T>
bool BlockDeque<T>::Empty() {
  std::lock_guard<std::mutex> locker(mutex_);
  return deq_.empty();
}
template<typename T>
bool BlockDeque<T>::Full() {
  std::lock_guard<std::mutex> locker(mutex_);
  return deq_.size() >= capacity_;
}
template<typename T>
bool BlockDeque<T>::Pop(T &item) {
  /*当队列为空的时候阻塞，直到被通知有数据入队*/
  std::unique_lock<std::mutex> locker(mutex_);
  while (deq_.empty()) {
    cond_consumer_.wait(locker);
    if (is_close_) {
      return false;
    }
  }
  item = deq_.front();
  deq_.pop_back();
  /*通知生产者已取走一个有空间，可以继续生产*/
  condition_variable_.notify_one();
  return true;
}
template<typename T>
bool BlockDeque<T>::Pop(T &item, int timeout) {
  /*Pop函数重载版本，可以设置阻塞事件(单位秒)*/
  std::unique_lock<std::mutex> locker(mutex_);
  while (deq_.empty()) {
    if (cond_consumer_.wait_for(locker, std::chrono::seconds(timeout)) ==
        std::cv_status::timeout) {
      return false;
    }
    if (is_close_) {
      return false;
    }
  }
  item = deq_.front();
  deq_.pop_front();
  /*通知生产者已取走一个有空间，可以继续生产*/
  condition_variable_.notify_one();
  return true;
}

#endif //WEBSERVER_SRC_LOG_BLOCKDEQUE_H_
