//
// Created by chenshihao on 2021/12/8.
//

#include "httprequest.h"

const unordered_set<string>HttpRequest::DEFAULT_HTML{
    "/index", "/register", "/registersuccess", "/login", "/loginsuccess", "/welcome", "/video", "/picture"
};
const unordered_map<string, int>HttpRequest::DEFAULT_HTML_TAG{
    {"/register.html", 0}, {"/login.html", 1}
};
void HttpRequest::Init() {
  method_ = path_ = version_ = body_ = "";
  state_ = REQUEST_LINE;
  header_.clear();
  post_.clear();
}
bool HttpRequest::IsKeepAlive() const {
  if (header_.count("Connection")) {
    /*header_是const变量，是无法使用【】取值*/
    return header_.at("Connection") == "keep-alive" && version_ == "1.1";
  }
  return false;
}

bool HttpRequest::Parse(Buffer &buff) {
  const char CRLF[] = "\r\n";
  if (buff.ReadableBytes() <= 0) {
    return false;
  }
  while (buff.ReadableBytes() && state_ != FINISH) {
    /*在buff找到第一个CRLF即一行的结束符号*/
    const char *lineEnd = search(buff.Peek(), buff.BeginWriteConst(), CRLF, CRLF + 2);
    /*获取一行*/
    string line(buff.Peek(), lineEnd);
    switch (state_) {
      case REQUEST_LINE:
        if (!ParseRequestLine(line)) {
          return false;
        }
        ParsePath_();
        break;
      case HEADERS:ParseHeader_(line);
        if (buff.ReadableBytes() <= 2) {
          state_ = FINISH;
        }
        break;
      case BODY:ParseBody_(line);
        break;
      default:break;
    }
    if (lineEnd == buff.BeginWrite()) { break; }
    buff.RetrieveUntil(lineEnd + 2);
  }
  LOG_DEBUG("[%s],[%s],[%s]", method_.c_str(), path_.c_str(), version_.c_str())
  return true;
}

void HttpRequest::ParsePath_() {
  if (path_ == "/") {
    path_ = "/index.html";
  } else {
    for (const auto &item: DEFAULT_HTML) {
      if (item == path_) {
        path_ += ".html";
        break;
      }
    }
  }
}

bool HttpRequest::ParseRequestLine(const string &line) {
  regex pattern("^([^ ]*) ([^ ]*) HTTP/([^ ]*)$");
  smatch subMatch;
  if (regex_match(line, subMatch, pattern)) {
    /*[0]是整个匹配成功的字符串*/
    method_ = subMatch[1];
    path_ = subMatch[2];
    version_ = subMatch[3];
    state_ = HEADERS;
    return true;
  }
  LOG_ERROR("RequestLine Error")
  return false;
}
void HttpRequest::ParseHeader_(const string &line) {
  regex pattern("^([^:]*): ?(.*)$");
  smatch subMatch;
  if (regex_match(line, subMatch, pattern)) {
    header_[subMatch[1]] = subMatch[2];
  } else {
    state_ = BODY;
  }
}

void HttpRequest::ParseBody_(const string &line) {
  body_ = line;
  ParsePost_();
  state_ = FINISH;
  LOG_DEBUG("Body:%s,len:%d", line.c_str(), line.size())
}
int HttpRequest::ConvertHex(char ch) {
  if (ch >= 'A' && ch <= 'F') return ch - 'A' + 10;
  if (ch >= 'a' && ch <= 'f') return ch - 'a' + 10;
  return ch;
}

void HttpRequest::ParsePost_() {
  /*application/x-www-form-urlencoded ： <form encType=””>中默认的encType，form表单数据被编码为key/value格式发送到服务器（表单默认的提交数据的格式）*/
  if (method_ == "POST" && header_["Content-Type"] == "application/x-www-form-urlencoded") {
    ParseFromUrlencoded_();
    if (DEFAULT_HTML_TAG.count(path_)) {
      int tag = DEFAULT_HTML_TAG.find(path_)->second;
      LOG_DEBUG("Tag:%d", tag)
      if (tag == 0 || tag == 1) {
        bool isLogin = (tag == 1);/*判断是否是登录页面的请求*/
        if (UserVerify(post_["username"], post_["password"], isLogin)) {
          path_ = isLogin ? "/loginsuccess.html" : "/registersuccess.html";
        } else {
          path_ = "/error.html";
        }
      }
    }
  }
}
void HttpRequest::ParseFromUrlencoded_() {
  /*解析post请求提交的数据，通常格式是key=value&key=value*/
  if (body_.empty()) { return; }
  string key, value;
  int num;
  int n = body_.size();
  int i = 0, j = 0;
  for (; i < n; i++) {
    char ch = body_[i];
    switch (ch) {
      case '=':key = body_.substr(j, i - j);
        j = i + 1;
        break;
      case '+':body_[i] = ' ';
        break;
      case '%':num = ConvertHex(body_[i + 1]) * 16 + ConvertHex(body_[i + 2]);
        body_[i + 2] = num % 10 + '0';
        body_[i + 1] = num / 10 + '0';
        i += 2;
        break;
      case '&':value = body_.substr(j, i - j);
        j = i + 1;
        post_[key] = value;
        LOG_DEBUG("%s = %s", key.c_str(), value.c_str())
        break;
      default:break;
    }
  }
  assert(j <= i);
  /*处理最后一个值*/
  if (post_.count(key) == 0 && j < i) {
    value = body_.substr(j, i - j);
    post_[key] = value;
  }
}
bool HttpRequest::UserVerify(const string &name, const string &pwd, bool isLogin) {
  if (name.empty() || pwd.empty()) { return false; }
  LOG_INFO("Verify name:%s pwd:%s", name.c_str(), pwd.c_str())
  MYSQL *sql;
  SqlConnRAII raii(&sql, SqlPool::Instance());
  assert(sql);

  bool flag = false;
  char order[256] = {0};
//  MYSQL_FIELD *fields = nullptr;
  MYSQL_RES *res = nullptr;

  if (!isLogin) { flag = true; }
  /*查询用户密码*/
  snprintf(order, 256, "SELECT username,password FROM user WHERE username='%s' LIMIT 1", name.c_str());
  LOG_DEBUG("%s", order)

  /*0查询成功*/
  if (mysql_query(sql, order)) {
    mysql_free_result(res);
    return false;
  }
  /*取出查询结果集放到res*/
  res = mysql_store_result(sql);
  /*查询结果的列数*/
//  mysql_num_fields(res);
//  fields = mysql_fetch_fields(res);
  /*获取一行数据*/
  while (MYSQL_ROW row = mysql_fetch_row(res)) {
    LOG_DEBUG("MYSQL ROW:%s %s", row[0], row[1])
    string password(row[1]);
    /*登录行为*/
    if (isLogin) {
      if (pwd == password) { flag = true; }
      else {
        flag = false;
        LOG_DEBUG("pwd error!")
      }
    } else {
      flag = false;
      LOG_DEBUG("user used!")
    }
  }
  mysql_free_result(res);

  /*注册账号*/
  if (!isLogin && flag) {
    LOG_DEBUG("register!")
    bzero(order, 256);
    snprintf(order, 256, "INSERT INTO user(username,password) VALUES('%s','%s')", name.c_str(), pwd.c_str());
    LOG_DEBUG("%s", order)
    if (mysql_query(sql, order)) {
      LOG_DEBUG("Insert error!")
      flag = false;
    }
    flag = true;
  }
  SqlPool::Instance()->FreeConn(sql);
  LOG_DEBUG("UserVerify success!")
  return flag;
}
string HttpRequest::Path() const {
  return path_;
}
string &HttpRequest::Path() {
  return path_;
}
string HttpRequest::Method() const {
  return method_;
}
string HttpRequest::Version() const {
  return version_;
}
string HttpRequest::GetPost(const string &key) const {
  assert(!key.empty());
  if (post_.count(key) == 1) {
    return post_.at(key);
  }
  return "";
}
string HttpRequest::GetPost(const char *key) const {
  assert(key != nullptr);
  if (post_.count(key) == 1) {
    return post_.at(key);
  }
  return "";
}