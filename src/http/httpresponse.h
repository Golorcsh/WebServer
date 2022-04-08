//
// Created by chenshihao on 2021/12/10.
//

#ifndef WEBSERVER_SRC_HTTP_HTTPRESPONSE_H_
#define WEBSERVER_SRC_HTTP_HTTPRESPONSE_H_
#include <unordered_map>
#include <fcntl.h>       // open
#include <unistd.h>      // close
#include <sys/stat.h>    // stat
#include <sys/mman.h>    // mmap, munmap

#include "../buffer/buffer.h"
#include "../log/log.h"
using namespace std;
class HttpResponse {
 public:
  HttpResponse();
  ~HttpResponse();

  void Init(const string &srcDir, string &path, bool isKeepAlive = false, int code = -1);
  void MakeResponse(Buffer &buff);
  void UnmapFile();
  char *File();
  size_t FileLen() const;
  void ErrorContent(Buffer &buff, string message);
  int Code() const { return code_; }
 private:
  void AddStateLine(Buffer &buff);
  void AddHeader(Buffer &buff);
  void AddContent(Buffer &buff);

  void ErrorHtml();
  string GetFileType();

  int code_;
  bool is_keep_alive_;

  string path_;
  string src_dir_;

  char *mm_file_;
  //用于存放文件的状态
  struct stat mm_file_stat_;

  static const unordered_map<string, string> SUFFIX_TYPE;
  static const unordered_map<int, string> CODE_STATUS;
  static const unordered_map<int, string> CODE_PATH;
};

#endif //WEBSERVER_SRC_HTTP_HTTPRESPONSE_H_
