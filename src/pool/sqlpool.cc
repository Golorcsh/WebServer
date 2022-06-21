//
// Created by chenshihao on 2021/12/4.
//

#include "sqlpool.h"
using namespace std;

SqlPool::SqlPool() = default;

SqlPool *SqlPool::Instance() {
  static SqlPool connPool;
  return &connPool;
}
/*!
 * @brief 初始化，连接数据库
 * @param host
 * @param port
 * @param user
 * @param pwd
 * @param dbName
 * @param connSize
 */
void SqlPool::Init(const char *host, int port, const char *user,
                   const char *pwd, const char *dbName, int connSize = 10) {
  assert(connSize > 0);
  for (int i = 0; i < connSize; ++i) {
    MYSQL *sql = nullptr;
    sql = mysql_init(sql); /*初始化数据库连接,会自动调用mysql_library_init*/
    if (!sql) {
      LOG_ERROR("MySql init error!")
      assert(sql);
    }
    /*连接数据库*/
    sql = mysql_real_connect(sql, host, user, pwd, dbName, port, nullptr, 0);
    if (!sql) {
      LOG_ERROR("MySql Connect error!")
    }
    conn_queue_.push(sql);
  }
  max_conn_ = connSize;
  sem_init(&semId_, 0, max_conn_);
}
/*!
 * 获得一个数据库连接
 * @return
 */
MYSQL *SqlPool::GetConn() {
  MYSQL *sql = nullptr;
  if (conn_queue_.empty()) {
    LOG_WARN("SqlPool busy!")
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
/*!
 * @brief 回收一个数据库连接
 * @param conn
 */
void SqlPool::FreeConn(MYSQL *conn) {
  /*将使用完的sql重新入对，并且信号量加一*/
  assert(conn);
  lock_guard<mutex> locker(mutex_);
  conn_queue_.push(conn);
  sem_post(&semId_);
}
/*!
 * 关闭数据库连接池
 */
void SqlPool::ClosePool() {
  lock_guard<mutex> locker(mutex_);
  while (!conn_queue_.empty()) {
    auto sql = conn_queue_.front();
    conn_queue_.pop();
    mysql_close(sql);
  }
  mysql_library_end();
}
/*!
 * @brief 获得数据库连接池可用数量
 * @return
 */
int SqlPool::GetFreeConnCount() {
  lock_guard<mutex> locker(mutex_);
  return conn_queue_.size();
}

SqlPool::~SqlPool() { ClosePool(); }
