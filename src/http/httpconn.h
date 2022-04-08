//
// Created by chenshihao on 2021/12/11.
//

#ifndef WEBSERVER_SRC_HTTP_HTTPCONN_H_
#define WEBSERVER_SRC_HTTP_HTTPCONN_H_

#include <sys/types.h>
#include <sys/uio.h>/*readv/writev*/
#include <arpa/inet.h>/*sockaddr_in*/
#include <cstdlib> /*atoi*/
#include <cerrno>

#include "../log/log.h"
#include "../pool/sqlpoolRAII.h"
#include "../buffer/buffer.h"
#include "httprequest.h"
#include "httpresponse.h"
using namespace std;
class HttpConn {
 public:
  HttpConn();
  ~HttpConn();

  void Init(int sockFd, const sockaddr_in &addr);
  ssize_t Read(int *saveErrno);
  ssize_t Write(int *saveErrno);
  void Close();
  int GetFd() const;
  int GetPort() const;

  const char *GetIP() const;
  sockaddr_in GetAddr() const;
  bool Process();

  int ToWriteBytes() {
    return iov_[0].iov_len + iov_[1].iov_len;
  }

  bool IsKeepAlive() const {
    return request_.IsKeepAlive();
  }

  static bool is_et_;
  static const char *src_dir_;
  static std::atomic<int> user_count_;

 private:
  int fd_;
  struct sockaddr_in addr_;
  bool is_close_;
  int iov_cnt_;
  struct iovec iov_[2];

  Buffer read_buff_;/*读缓冲区*/
  Buffer write_buff_;/*写缓冲区*/

  HttpRequest request_;
  HttpResponse response_;
};

#endif //WEBSERVER_SRC_HTTP_HTTPCONN_H_
