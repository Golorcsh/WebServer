//
// Created by chenshihao on 2021/12/6.
//

#ifndef WEBSERVER_SRC_POOL_THREADPOOL_H_
#define WEBSERVER_SRC_POOL_THREADPOOL_H_

#include <mutex>
#include <condition_variable>
#include <queue>
#include <thread>
#include <functional>
class ThreadPool {
 public:
  explicit ThreadPool(size_t threadCount = 8) : pool_(std::make_shared<Pool>()) {
    assert(threadCount > 0);
    /*使用lambada表达式做为worker函数*/
    /*TODO 改写单独的worker函数*/
    for (int i = 0; i < threadCount; ++i) {
      std::thread([pool = pool_] {
        std::unique_lock<std::mutex> locker(pool->mtx);
        while (true) {
          /*若任务队列不为空，则取出任务执行*/
          if (!pool->tasks.empty()) {
            auto task = std::move(pool->tasks.front());
            pool->tasks.pop();
            locker.unlock();
            task();
            locker.lock();
          } else if (pool->isClosed) break;
          else pool->cond.wait(locker);
        }
      }).detach();
    }
  }
  ThreadPool() = default;
  ThreadPool(ThreadPool &&) = default;
  ~ThreadPool() {
    if (static_cast<bool>(pool_)) {
      {
        std::lock_guard<std::mutex> locker(pool_->mtx);
        pool_->isClosed = true;
      }
      pool_->cond.notify_all();/*唤醒所有线程，让线程自行销毁*/
    }
  }
  template<typename F>
  void AddTask(F &&task) {
    {
      std::lock_guard<std::mutex> locker(pool_->mtx);
      pool_->tasks.emplace(std::forward<F>(task));
    }
    pool_->cond.notify_one();/*唤醒线程执行任务*/
  }
 private:
  struct Pool {
    std::mutex mtx;
    std::condition_variable cond;
    bool isClosed;
    std::queue<std::function<void()>> tasks;
  };
  std::shared_ptr<Pool> pool_;
};
#endif //WEBSERVER_SRC_POOL_THREADPOOL_H_
