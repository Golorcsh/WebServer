//
// Created by chenshihao on 2021/12/6.
//

#ifndef WEBSERVER_SRC_POOL_SQLPOOLRAII_H_
#define WEBSERVER_SRC_POOL_SQLPOOLRAII_H_
#include "sqlpool.h"
/* 资源在对象构造初始化 资源在对象析构时释放*/
/*通过局部变量来管理资源*/
/*Resource Acquisition Is Initialization*/
class SqlConnRAII {
 public:
  SqlConnRAII(MYSQL **sql, SqlConnPool *connpool) {
    assert(connpool);
    *sql = connpool->GetConn();
    sql_ = *sql;
    connpool_ = connpool;
  }

  /*若获取了资源，当析构时执行释放资源*/
  ~SqlConnRAII() {
    if (sql_) { connpool_->FreeConn(sql_); }
  }
 private:
  MYSQL *sql_;
  SqlConnPool *connpool_;
};
#endif //WEBSERVER_SRC_POOL_SQLPOOLRAII_H_
