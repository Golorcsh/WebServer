//
// Created by chenshihao on 2021/12/4.
//

#include "sqlpool.h"
SqlPool::SqlPool() : use_count_(0), free_count_(0) {
}
SqlPool *SqlPool::Instance() {
  static SqlPool connPool;
  return &connPool;
}
void SqlPool::Init(const char *host, int port,
                   const char *user, const char *pwd, const char *dbName,
                   int connSize = 10) {
  assert(connSize > 0);
  for (int i = 0; i < connSize; ++i) {
    MYSQL *sql = nullptr;
    sql = mysql_init(sql);/*初始化数据库连接,会自动调用mysql_library_init*/
    if (!sql) {
      LOG_ERROR("Mysql init Error!");
      assert(sql);
    }
    /*连接数据库*/
    sql = mysql_real_connect(sql, host, user, pwd, dbName, port, nullptr, 0);
    if (!sql) {
      LOG_ERROR("MySql connect error!");
    }
    conn_queue_.push(sql);
  }
  max_conn_ = connSize;
  sem_init(&semId_, 0, max_conn_);
}
MYSQL *SqlPool::GetConn() {
  MYSQL *sql = nullptr;
  if (conn_queue_.empty()) {
    LOG_WARN("SqlConnPool busy!");
    return nullptr;
  }
  /*信号量减一*/
  sem_wait(&semId_);
  {
    std::lock_guard<std::mutex> locker(mutex_);
    sql = conn_queue_.front();
    conn_queue_.pop();
  }
  return sql;
}
void SqlPool::FreeConn(MYSQL *sql) {
  /*将使用完的sql重新入对，并且信号量加一*/
  assert(sql);
  std::lock_guard<std::mutex> locker(mutex_);
  conn_queue_.push(sql);
  sem_post(&semId_);
}
int SqlPool::GetFreeConnCount() {
  std::lock_guard<std::mutex> locker(mutex_);
  return conn_queue_.size();
}
void SqlPool::ClosePool() {
  std::lock_guard<std::mutex> locker(mutex_);
  while (!conn_queue_.empty()) {
    auto sql = conn_queue_.front();
    conn_queue_.pop();
    mysql_close(sql);
  }
  mysql_library_end();
}
SqlPool::~SqlPool() {
  ClosePool();
}

