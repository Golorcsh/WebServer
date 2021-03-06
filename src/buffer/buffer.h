//
// Created by chenshihao on 2021/12/4.
//

#ifndef WEBSERVER_SRC_BUFFER_H_
#define WEBSERVER_SRC_BUFFER_H_
#include <assert.h>
#include <atomic>
#include <cstring> //perror
#include <iostream>
#include <sys/uio.h> //readv
#include <unistd.h>  // write
#include <vector>    //readv
class Buffer {
public:
  Buffer(int initBuffSize = 1024);
  ~Buffer() = default;

  size_t WritableBytes() const;
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

  ssize_t ReadFd(int fd, int *Errno);
  ssize_t WriteFd(int fd, int *Errno);

private:
  char *BeginPtr_();
  const char *BeginPtr_() const;
  void MakeSpace_(size_t len);

  std::vector<char> buffer_;
  /*读取位置*/
  std::atomic<size_t> read_pos_;
  /*写入位置*/
  std::atomic<size_t> write_pos_;
};

#endif // WEBSERVER_SRC_BUFFER_H_
