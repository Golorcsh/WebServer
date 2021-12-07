//
// Created by chenshihao on 2021/12/4.
//

#ifndef WEBSERVER_SRC_POOL_SQLPOOL_H_
#define WEBSERVER_SRC_POOL_SQLPOOL_H_
#include <mysql//mysql.h>
#include <string>
#include <queue>
#include <mutex>
#include <semaphore.h>
#include <thread>
#include "../log/log.h"

class SqlPool {
 public:
  static SqlPool *Instance();

  MYSQL *GetConn();
  void FreeConn(MYSQL *sql);
  int GetFreeConnCount();

  void Init(const char *host, int port,
            const char *user, const char *pwd,
            const char *dbName, int connSize);
  void ClosePool();
 private:
  SqlPool();
  ~SqlPool();

  int max_conn_;/*最大连接数*/
  int use_count_;/*已使用连接数*/
  int free_count_;/*空闲连接数*/

  std::queue<MYSQL *> conn_queue_;
  std::mutex mutex_;
  sem_t semId_;
};

#endif //WEBSERVER_SRC_POOL_SQLPOOL_H_
