//
// Created by chenshihao on 2021/12/13.
//

#ifndef WEBSERVER_SRC_SERVER_WEBSERVER_H_
#define WEBSERVER_SRC_SERVER_WEBSERVER_H_
#include <unordered_map>
#include <fcntl.h>
#include <unistd.h>
#include <cassert>
#include <cerrno>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "epoller.h"
#include "../log/log.h"
#include "../timer/heaptimer.h"
#include "../pool/sqlpool.h"
#include "../pool/sqlpoolRAII.h"
#include "../pool/threadpool.h"
#include "../http/httpconn.h"

class WebServer {
 public:
  WebServer(int port, int trigMode, int timeoutMs, bool OptLinger,
            int sqlPort, const char *sqlUser, const char *sqlPwd,
            const char *dbName, int connPoolNum, int threadNum,
            bool openLog, int logLevel, int logQueSize);

  ~WebServer();
  void Start();
 private:
  bool InitSocket_();
  void InitEventMode_(int trigMode);
  void AddClient_(int fd, sockaddr_in addr);

  void DealListen_();
  void DealWrite_(HttpConn *client);
  void DealRead_(HttpConn *client);

  void SendError(int fd, const char *info);
  void ExtentTime_(HttpConn *client);
  void CloseConn_(HttpConn *client);

  void OnRead_(HttpConn *client);
  void OnWrite_(HttpConn *client);
  void OnProcess_(HttpConn *client);

  static const int MAX_FD = 65536;

  static int SetFdNoneBlocking(int fd);

  int port_;
  bool openLinger_;
  int timeoutMs_;
  bool isClose_;
  int listenFd_;
  char *srcDir_;

  uint32_t listenEvent_;
  uint32_t connEvent_;

  std::unique_ptr<HeapTimer> timer_;
  std::unique_ptr<ThreadPool> threadpool_;
  std::unique_ptr<Epoller> epoller_;
  std::unordered_map<int, HttpConn> user_;
};

#endif //WEBSERVER_SRC_SERVER_WEBSERVER_H_
