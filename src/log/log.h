//
// Created by chenshihao on 2021/12/4.
//

#ifndef WEBSERVER_SRC_LOG_LOG_H_
#define WEBSERVER_SRC_LOG_LOG_H_
#include <mutex>
#include <string>
#include <thread>
#include <sys/time.h>
#include <string.h>
#include <stdarg.h>           // vastart va_end
#include <assert.h>
#include <sys/stat.h>         //mkdir
#include "blockdeque.h"
#include "../buffer/buffer.h"
class Log {
 public:
  void Init(int level, const char *path = "./log",
            const char *suffix = ".log",
            int maxQueueCapacity = 1024);

  static Log *Instance();
  static void FlushLogThread();

  void Write(int level, const char *format, ...);
  void Flush();

  int GetLevel();
  void SetLevel(int level);
  bool IsOpen() {
    return isOpen_;
  }
 private:
  Log();
  void AppendLogLevelTitle_(int level);
  virtual ~Log();
  void AsyncWrite_();
 private:
  static const int LOG_PATH_LEN = 256;
  static const int LOG_NAME_LEN = 256;
  static const int MAX_LINES = 50000;

  const char *path_;
  const char *suffix_;

  int max_line_;

  int line_count_;
  int toDay_;
  bool isOpen_;

  Buffer buff_;

  int level_;

  bool isAsync_;
  FILE *fp_;
  std::unique_ptr<BlockDeque<std::string>> deque_;
  std::unique_ptr<std::thread> writeThread_;
  std::mutex mutex_;
};

#define LOG_BASE(level, format, ...) \
  do{\
      Log* log = Log::Instance();\
      if(log->IsOpen()&&log->GetLevel()<=level){\
            log->Write(level, format, ##__VA_ARGS__); \
            log->Flush();\
      }\
    }while(0);

#define LOG_DEBUG(format, ...) do {LOG_BASE(0, format, ##__VA_ARGS__)} while(0);
#define LOG_INFO(format, ...) do {LOG_BASE(1, format, ##__VA_ARGS__)} while(0);
#define LOG_WARN(format, ...) do {LOG_BASE(2, format, ##__VA_ARGS__)} while(0);
#define LOG_ERROR(format, ...) do {LOG_BASE(3, format, ##__VA_ARGS__)} while(0);

#endif //WEBSERVER_SRC_LOG_LOG_H_
