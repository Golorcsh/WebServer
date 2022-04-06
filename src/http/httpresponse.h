//
// Created by chenshihao on 2021/12/10.
//

#ifndef WEBSERVER_SRC_HTTP_HTTPRESPONSE_H_
#define WEBSERVER_SRC_HTTP_HTTPRESPONSE_H_
#include <unordered_map>
#include <unistd.h> /*close*/
#include <fcntl.h>
#include <sys/stat.h> /*close*/
#include <sys/mman.h> /*mmap,unmap,将一段内存映射到内核空间*/

#include "../buffer/buffer.h"
#include "../log/log.h"

class HttpResponse {
 public:
  HttpResponse();
  ~HttpResponse();

  void Init(const std::string &srcDir, std::string path, bool isKeepAlive = false, int code = -1);
  void MakeResponse(Buffer &buff);
  void UnmapFile();
  char *File();
  size_t FileLen() const;
  void ErrorContent(Buffer &buff, const std::string &message);
  int Code() const { return code_; }
 private:
  void AddStateLine_(Buffer &buff);
  void AddHeader_(Buffer &buff);
  void AddContent_(Buffer &buff);

  void ErrorHtml_();
  std::string GetFileType_();

  int code_;
  bool isKeepAlive_;

  std::string path_;
  std::string srcDir_;

  char *mmFile_;
  //用于存放文件的状态
  struct stat mmFileStat_;

  static const std::unordered_map<std::string, std::string> SUFFIX_TYPE;
  static const std::unordered_map<int, std::string> CODE_STATUS;
  static const std::unordered_map<int, std::string> CODE_PATH;
};

#endif //WEBSERVER_SRC_HTTP_HTTPRESPONSE_H_
