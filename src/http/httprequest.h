//
// Created by chenshihao on 2021/12/8.
//

#ifndef WEBSERVER_SRC_HTTP_HTTPREQUEST_H_
#define WEBSERVER_SRC_HTTP_HTTPREQUEST_H_
#include <unordered_map>
#include <unordered_set>
#include <string>
#include <regex>
#include <errno.h>
#include <mysql/mysql.h>

#include "../buffer/buffer.h"
#include "../log/log.h"
#include "../pool/sqlpool.h"
#include "../pool/sqlpoolRAII.h"

class HttpRequest {
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

  std::string Path() const;
  std::string &Path();
  std::string Method() const;
  std::string Version() const;
  std::string GetPost(const std::string &key) const;
  std::string GetPost(const char *key) const;

  bool IsKeepAlive() const;
 private:
  bool ParseRequestLine(const std::string &line);
  void ParseHeader_(const std::string &line);
  void ParseBody_(const std::string &line);

  void ParsePath_();
  void ParsePost_();
  void ParseFromUrlencoded_();
  static bool UserVerify(const std::string &name, const std::string &pwd, bool isLogin);

  PARSE_STATE state_;
  std::string method_, path_, version_, body_;
  std::unordered_map<std::string, std::string> header_;
  std::unordered_map<std::string, std::string> post_;

  static const std::unordered_set<std::string> DEFAULT_HTML;
  static const std::unordered_map<std::string, int> DEFAULT_HTML_TAG;
  static int ConvertHex(char ch);
};

#endif //WEBSERVER_SRC_HTTP_HTTPREQUEST_H_
