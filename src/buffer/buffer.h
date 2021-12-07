//
// Created by chenshihao on 2021/12/4.
//

#ifndef WEBSERVER_SRC_BUFFER_H_
#define WEBSERVER_SRC_BUFFER_H_
#include <cstring>/*perror*/
#include <iostream>
#include <unistd.h>/*write*/
#include <sys/uio.h>/*readv 块读取*/
#include <vector>
#include <atomic>
#include <cassert>
class Buffer {
 public:
  explicit Buffer(int init_buffer_size = 1024);
  ~Buffer() = default;

  size_t WriteableBytes() const;
  size_t ReadableBytes() const;
  size_t PrependableBytes() const;

  const char *Peek() const;
  void EnsureWriteable(size_t len);
  void HasWritten(size_t len);

  void Retrieve(size_t len);
  void RetrieveUntil(const char *end);

  void RetrieveAll();
  std::string RetrieveAllToStr();

  const char *BeginWriteConst() const;
  char *BeginWrite();

  void Append(const std::string &str);
  void Append(const char *str, size_t len);
  void Append(const void *data, size_t len);
  void Append(const Buffer &buff);

  ssize_t ReadFD(int fd, int *Errno);
  ssize_t WriteFD(int fd, int *Erron);
 private:
  char *BeginPtr();
  const char *BeginPtr() const;
  void MakeSpace_(size_t len);

  std::vector<char> buffer_;
  /*读取位置*/
  std::atomic<size_t> readPos_;
  /*写入位置*/
  std::atomic<size_t> writePos_;
};

#endif //WEBSERVER_SRC_BUFFER_H_
