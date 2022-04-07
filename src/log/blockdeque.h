//
// Created by chenshihao on 2021/12/4.
//

#ifndef WEBSERVER_SRC_LOG_BLOCKDEQUE_H_
#define WEBSERVER_SRC_LOG_BLOCKDEQUE_H_
#include <mutex>
#include <deque>
#include <condition_variable>
#include <sys/time.h>

template<typename T>
class BlockDeque {
 public:
    explicit BlockDeque(size_t MaxCapacity = 1000);

  ~BlockDeque();
  void clear();
  bool empty();
  bool full();
  void Close();
  size_t size();
  size_t capacity();
  T front();
  T back();
  void push_back(const T &item);
  void push_front(const T &item);
  bool pop(T &item);
  bool pop(T &item, int timeout);
  void flush();
 private:
  std::deque<T> deq_;
  size_t capacity_;
  std::mutex mtx_;
  bool isClose_;
  std::condition_variable condConsumer_;
  std::condition_variable condProducer_;
};
template<typename T>
BlockDeque<T>::BlockDeque(size_t MaxCapacity):capacity_(MaxCapacity) {
  assert(MaxCapacity > 0);
  isClose_ = false;
}
template<typename T>
BlockDeque<T>::~BlockDeque() {
  Close();
}
template<typename T>
void BlockDeque<T>::Close() {
  /*上锁，清空队列，并设置is_close为true*/
  {
    std::lock_guard<std::mutex> locker(mtx_);
    deq_.clear();
    isClose_ = true;
  }
  /*发送通知*/
  condProducer_.notify_all();
  condConsumer_.notify_all();
}
template<typename T>
void BlockDeque<T>::flush() {
  condConsumer_.notify_one();
}
template<typename T>
void BlockDeque<T>::clear() {
  std::lock_guard<std::mutex> locker(mtx_);
  deq_.clear();
}
template<typename T>
T BlockDeque<T>::front() {
  std::lock_guard<std::mutex> locker(mtx_);
  return deq_.front();
}
template<typename T>
T BlockDeque<T>::back() {
  std::lock_guard<std::mutex> locker(mtx_);
  return deq_.back();
}
template<typename T>
size_t BlockDeque<T>::size() {
  std::lock_guard<std::mutex> locker(mtx_);
  return deq_.size();
}
template<typename T>
size_t BlockDeque<T>::capacity() {
    std::lock_guard<std::mutex> locker(mtx_);
  return capacity_;
}
template<typename T>
void BlockDeque<T>::push_back(const T &item) {
  /*上锁*/
  std::unique_lock<std::mutex> locker(mtx_);
  /*当队列大小超出设置的最大容量是，阻塞*/
  while (deq_.size() >= capacity_) {
    condProducer_.wait(locker);
  }
  deq_.push_back(item);
  /*通知消费者队列中有数据，可以取*/
  condConsumer_.notify_one();
}
template<typename T>
void BlockDeque<T>::push_front(const T &item) {
  /*上锁*/
  std::unique_lock<std::mutex> locker(mtx_);
  /*当队列大小超出设置的最大容量是，阻塞*/
  while (deq_.size() >= capacity_) {
    condProducer_.wait(locker);
  }
  deq_.push_front(item);
  /*通知消费者队列中有数据，可以取*/
  condConsumer_.notify_one();
}
template<typename T>
bool BlockDeque<T>::empty() {
  std::lock_guard<std::mutex> locker(mtx_);
  return deq_.empty();
}
template<typename T>
bool BlockDeque<T>::full() {
  std::lock_guard<std::mutex> locker(mtx_);
  return deq_.size() >= capacity_;
}
template<typename T>
bool BlockDeque<T>::pop(T &item) {
  /*当队列为空的时候阻塞，直到被通知有数据入队*/
  std::unique_lock<std::mutex> locker(mtx_);
  while (deq_.empty()) {
    condConsumer_.wait(locker);
    if (isClose_){
      return false;
      }
  }
  item = deq_.front();
  deq_.pop_back();
  /*通知生产者已取走一个有空间，可以继续生产*/
  condProducer_.notify_one();
  return true;
}
template<typename T>
bool BlockDeque<T>::pop(T &item, int timeout) {
  /*Pop函数重载版本，可以设置阻塞事件(单位秒)*/
  std::unique_lock<std::mutex> locker(mtx_);
  while (deq_.empty()) {
    if (condConsumer_.wait_for(locker, std::chrono::seconds(timeout)) ==
        std::cv_status::timeout) {
      return false;
    }
    if (isClose_){
      return false;
      }
  }
  item = deq_.front();
  deq_.pop_front();
  /*通知生产者已取走一个有空间，可以继续生产*/
  condProducer_.notify_one();
  return true;
}

#endif //WEBSERVER_SRC_LOG_BLOCKDEQUE_H_
