//
// Created by chenshihao on 2021/12/4.
//

#include "sqlpool.h"
using namespace std;

SqlConnPool::SqlConnPool() : use_count_(0), free_count_(0) {
}

SqlConnPool *SqlConnPool::Instance() {
  static SqlConnPool connPool;
  return &connPool;
}
void SqlConnPool::Init(const char *host, int port,
                       const char *user, const char *pwd, const char *dbName,
                       int connSize = 10) {
  assert(connSize > 0);
  for (int i = 0; i < connSize; ++i) {
    MYSQL *sql = nullptr;
    sql = mysql_init(sql);/*初始化数据库连接,会自动调用mysql_library_init*/
    if (!sql) {
            LOG_ERROR("MySql init error!");
      assert(sql);
    }
    /*连接数据库*/
    sql = mysql_real_connect(sql, host, user, pwd, dbName, port, nullptr, 0);
    if (!sql) {
            LOG_ERROR("MySql Connect error!");
    }
    conn_queue_.push(sql);
  }
  max_conn_ = connSize;
  sem_init(&semId_, 0, max_conn_);
}
MYSQL *SqlConnPool::GetConn() {
  MYSQL *sql = nullptr;
  if (conn_queue_.empty()) {
    LOG_WARN("SqlConnPool busy!");
    return nullptr;
  }
  /*信号量减一*/
  sem_wait(&semId_);
  {
    lock_guard<mutex> locker(mutex_);
    sql = conn_queue_.front();
    conn_queue_.pop();
  }
  return sql;
}
void SqlConnPool::FreeConn(MYSQL *conn) {
  /*将使用完的sql重新入对，并且信号量加一*/
  assert(conn);
  lock_guard<mutex> locker(mutex_);
  conn_queue_.push(conn);
  sem_post(&semId_);
}

void SqlConnPool::ClosePool() {
  lock_guard<mutex> locker(mutex_);
  while (!conn_queue_.empty()) {
    auto sql = conn_queue_.front();
    conn_queue_.pop();
    mysql_close(sql);
  }
  mysql_library_end();
}

int SqlConnPool::GetFreeConnCount() {
  lock_guard<mutex> locker(mutex_);
  return conn_queue_.size();
}

SqlConnPool::~SqlConnPool() {
  ClosePool();
}

