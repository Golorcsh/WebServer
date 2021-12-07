//
// Created by chenshihao on 2021/12/6.
//

#ifndef WEBSERVER_SRC_POOL_SQLPOOLRAII_H_
#define WEBSERVER_SRC_POOL_SQLPOOLRAII_H_
#include "sqlpool.h"
/* 资源在对象构造初始化 资源在对象析构时释放*/
/*通过局部变量来管理资源*/
/*Resource Acquisition Is Initialization*/
class SqlPoolRAII {
 public:
  /*构造函数时，获取资源*/
  SqlPoolRAII(MYSQL **sql, SqlPool *sqlpool) {
    assert(sqlpool);
    *sql = sqlpool->GetConn();
    sql_ = *sql;
    sql_pool_ = sqlpool;
  }

  /*若获取了资源，当析构时执行释放资源*/
  ~SqlPoolRAII() {
    if (sql_) {
      sql_pool_->FreeConn(sql_);
    }
  }
 private:
  MYSQL *sql_;
  SqlPool *sql_pool_;
};
#endif //WEBSERVER_SRC_POOL_SQLPOOLRAII_H_
