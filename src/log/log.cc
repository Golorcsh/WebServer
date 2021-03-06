//
// Created by chenshihao on 2021/12/4.
//

#include "log.h"

using namespace std;

Log::Log() {
  line_count_ = 0;
  is_async_ = false;
  write_thread_ = nullptr;
  deque_ = nullptr;
  to_day_ = 0;
  fp_ = nullptr;
}
Log::~Log() {
  /*若线程还在，则等待线程结束，并且关闭blockdeque_*/
  if (write_thread_ && write_thread_->joinable()) {
    while (!deque_->Empty()) {
      deque_->Flush();
    }
    deque_->Close();
    write_thread_->join();
  }
  /*关闭打开的文件*/
  if (fp_) {
    lock_guard<mutex> locker(mutex_);
    Flush();
    fclose(fp_);
  }
}
int Log::GetLevel() {
  lock_guard<mutex> locker(mutex_);
  return level_;
}
void Log::SetLevel(int level) {
  lock_guard<mutex> locker(mutex_);
  level_ = level;
}
void Log::Init(int level = 1, const char *path, const char *suffix,
               int maxQueueCapacity) {
  is_open_ = true;
  level_ = level;
  if (maxQueueCapacity > 0) {
    is_async_ = true;
    if (!deque_) {
      /*创建deque_*/
      unique_ptr<BlockDeque<string>> newDeque(new BlockDeque<string>);
      deque_ = move(newDeque);

      unique_ptr<thread> newThread(new thread(FlushLogThread));
      write_thread_ = move(newThread);
    }
  } else {
    is_async_ = false;
  }

  line_count_ = 0;
  /*获得系统时间*/
  time_t timer = time(nullptr);
  struct tm *sysTime = localtime(&timer);
  struct tm t = *sysTime;

  path_ = path;
  suffix_ = suffix;
  char fileName[LOG_NAME_LEN] = {0};
  snprintf(fileName, LOG_NAME_LEN - 1, "%s/%04d_%02d_%02d%s",
           path_, t.tm_year + 1900, t.tm_mon + 1, t.tm_mday, suffix_);
  to_day_ = t.tm_mday;

  {
    lock_guard<mutex> locker(mutex_);
    /*清空buff*/
    buff_.RetrieveAll();
    /*如果fp_已经打开，则关闭*/
    if (fp_) {
      Flush();
      fclose(fp_);
    }
    /*打开log文件*/
    fp_ = fopen(fileName, "a");
    if (fp_ == nullptr) {
      mkdir(path_, 0777);
      fp_ = fopen(fileName, "a");
    }
    assert(fp_ != nullptr);
  }
}
void Log::Write(int level, const char *format, ...) {
  struct timeval now = {0, 0};
  /*获得当前事件的秒数,从1970年开始计算*/
  gettimeofday(&now, nullptr);
  /*通过秒数获得当前时间*/
  time_t tSec = now.tv_sec;
  struct tm *sysTime = localtime(&tSec);
  struct tm t = *sysTime;
  /*用于处理可变参数(variable arguments)*/
  va_list va_list;

  /*日志日期，日志行数,若不是同一天则新建一个log文件*/
  if (to_day_ != t.tm_mday || (line_count_ && (line_count_ % MAX_LINES == 0))) {
    unique_lock<mutex> locker(mutex_);
    locker.unlock();

    char newFile[LOG_NAME_LEN];
    char tail[36] = {0};
    snprintf(tail, 36, "%04d_%02d_%02d", t.tm_year + 1900, t.tm_mon + 1, t.tm_mday);

    if (to_day_ != t.tm_mday) {
      snprintf(newFile, LOG_NAME_LEN - 72, "%s/%s%s", path_, tail, suffix_);
      to_day_ = t.tm_mday;
      line_count_ = 0;
    } else {
      snprintf(newFile, LOG_NAME_LEN - 72, "%s/%s-%d%s", path_, tail, (line_count_ / MAX_LINES), suffix_);
    }

    locker.lock();
    Flush();
    fclose(fp_);
    fp_ = fopen(newFile, "a");
    assert(fp_ != nullptr);
  }

  {
    unique_lock<mutex> locker(mutex_);
    line_count_++;
    /*向缓存写入日期*/
    int n = snprintf(buff_.BeginWrite(), 128, "%d-%02d-%02d %02d:%02d:%02d.%06ld",
                     t.tm_year + 1900, t.tm_mon + 1, t.tm_mday,
                     t.tm_hour, t.tm_min, t.tm_sec, now.tv_usec);
    buff_.HasWritten(n);
    /*插入log级别信息*/
    AppendLogLevelTitle_(level);

    /*写入format和可变参数*/
    va_start(va_list, format);
    int m = vsnprintf(buff_.BeginWrite(), buff_.WritableBytes(), format, va_list);
    va_end(va_list);

    buff_.HasWritten(m);
    buff_.Append("\n\0", 2);

    /*异步的方式，将buff中的数据以string方式取出，并放到block队列中*/
    if (is_async_ && deque_ && !deque_->Full()) {
      deque_->PushBack(buff_.RetrieveAllToStr());
    } else {
      /*直接写入到long文件中*/
      fputs(buff_.Peek(), fp_);
    }
    /*清空buff_*/
    buff_.RetrieveAll();
  }
}
void Log::AppendLogLevelTitle_(int level) {
  switch (level) {
    case 0:buff_.Append("[debug]: ", 9);
      break;
    case 1:buff_.Append("[info] : ", 9);
      break;
    case 2:buff_.Append("[warn] : ", 9);
      break;
    case 3:buff_.Append("[error]: ", 9);
      break;
    default:buff_.Append("[info] : ", 9);
      break;
  }
}
void Log::Flush() {
  if (is_async_) {
    deque_->Flush();
  }
  fflush(fp_);
}
void Log::AsyncWrite_() {
  string str;
  while (deque_->Pop(str)) {
    lock_guard<mutex> locker(mutex_);
    fputs(str.c_str(), fp_);
  }
}
Log *Log::Instance() {
  static Log instance;
  return &instance;
}
void Log::FlushLogThread() {
  Log::Instance()->AsyncWrite_();
}