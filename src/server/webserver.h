//
// Created by chenshihao on 2021/12/13.
//

#ifndef WEBSERVER_SRC_SERVER_WEBSERVER_H_
#define WEBSERVER_SRC_SERVER_WEBSERVER_H_
#include <arpa/inet.h>
#include <cassert>
#include <cerrno>
#include <fcntl.h> // fcntl()
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h> // close()
#include <unordered_map>

#include "../http/httpconn.h"
#include "../log/log.h"
#include "../pool/sqlpool.h"
#include "../pool/sqlpoolRAII.h"
#include "../pool/threadpool.h"
#include "../timer/heaptimer.h"
#include "epoller.h"

class WebServer {
public:
  WebServer(int port, int trigMode, int timeoutMS, bool OptLinger, int sqlPort,
            const char *sqlUser, const char *sqlPwd, const char *dbName,
            int connPoolNum, int threadNum, bool openLog, int logLevel,
            int logQueSize);

  ~WebServer();
  void Start();

private:
  bool InitSocket_();
  void InitEventMode_(int trigMode);
  void AddClient_(int fd, sockaddr_in addr);

  void DealListen_();
  void DealWrite_(HttpConn *client);
  void DealRead_(HttpConn *client);

  static void SendError_(int fd, const char *info);
  void ExtentTime_(HttpConn *client);
  void CloseConn_(HttpConn *client);

  void OnRead_(HttpConn *client);
  void OnWrite_(HttpConn *client);
  void OnProcess(HttpConn *client);

  static const int MAX_FD = 65536;

  static int SetFdNoneBlock(int fd);

  int port_;
  bool openLinger_;
  int timeoutMS_; /* 毫秒MS */
  bool isClose_;
  int listenFd_{};
  char *srcDir_;

  uint32_t listenEvent_{};
  uint32_t connEvent_{};

  // 使用智能指针管理定时器堆
  std::unique_ptr<HeapTimer> timer_;
  // 使用智能指针管理线程池
  std::unique_ptr<ThreadPool> thread_pool_;
  // 使用智能指针管理封装好的epoll
  std::unique_ptr<Epoller> epoller_;
  // key为fd,value为对应的链接
  std::unordered_map<int, HttpConn> users_;
};

#endif // WEBSERVER_SRC_SERVER_WEBSERVER_H_
