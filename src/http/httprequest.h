//
// Created by chenshihao on 2021/12/8.
//

#ifndef WEBSERVER_SRC_HTTP_HTTPREQUEST_H_
#define WEBSERVER_SRC_HTTP_HTTPREQUEST_H_
#include <unordered_map>
#include <unordered_set>
#include <string>
#include <regex>
#include <cerrno>
#include <mysql/mysql.h>

#include "../buffer/buffer.h"
#include "../log/log.h"
#include "../pool/sqlpool.h"
#include "../pool/sqlpoolRAII.h"
using namespace std;
class HttpRequest {
 public:
  enum PARSE_STATE {
    REQUEST_LINE,
    HEADERS,
    BODY,
    FINISH
  };

  enum HTTP_CODE {
    NO_REQUEST = 0,
    GET_REQUEST,
    BAD_REQUEST,
    NO_RESOURCE,
    FORBIDDEN_REQUEST,
    FILE_REQUEST,
    INTERNAL_ERROR,
    CLOSED_CONNECTION,
  };

  HttpRequest() { Init(); }
  ~HttpRequest() = default;

  void Init();
  bool Parse(Buffer &buff);

  string Path() const;
  string &Path();
  string Method() const;
  string Version() const;
  string GetPost(const string &key) const;
  string GetPost(const char *key) const;

  bool IsKeepAlive() const;
 private:
  bool ParseRequestLine(const string &line);
  void ParseHeader_(const string &line);
  void ParseBody_(const string &line);

  void ParsePath_();
  void ParsePost_();
  void ParseFromUrlencoded_();
  static bool UserVerify(const string &name, const string &pwd, bool isLogin);

  PARSE_STATE state_;
  string method_, path_, version_, body_;
  unordered_map<string, string> header_;
  unordered_map<string, string> post_;

  static const unordered_set<string> DEFAULT_HTML;
  static const unordered_map<string, int> DEFAULT_HTML_TAG;
  static int ConvertHex(char ch);
};

#endif //WEBSERVER_SRC_HTTP_HTTPREQUEST_H_
