//
// Created by chenshihao on 2021/12/10.
//

#include "httpresponse.h"

const std::unordered_map<std::string, std::string> HttpResponse::SUFFIX_TYPE = {
    {".html", "text/html"},
    {".xml", "text/xml"},
    {".xhtml", "application/xhtml+xml"},
    {".txt", "text/plain"},
    {".rtf", "application/rtf"},
    {".pdf", "application/pdf"},
    {".word", "application/nsword"},
    {".png", "image/png"},
    {".gif", "image/gif"},
    {".jpg", "image/jpeg"},
    {".jpeg", "image/jpeg"},
    {".au", "audio/basic"},
    {".mpeg", "video/mpeg"},
    {".mpg", "video/mpeg"},
    {".avi", "video/x-msvideo"},
    {".gz", "application/x-gzip"},
    {".tar", "application/x-tar"},
    {".css", "text/css "},
    {".js", "text/javascript "},
};
const std::unordered_map<int, std::string> HttpResponse::CODE_STATUS = {
    {200, "OK"},
    {400, "Bad Request"},
    {403, "Forbidden"},
    {404, "Not Found"},
};
const std::unordered_map<int, std::string> HttpResponse::CODE_PATH = {
    {400, "/400.html"},
    {403, "/403.html"},
    {404, "/404.html"},
};
HttpResponse::HttpResponse() {
  code_ = -1;
  path_ = srcDir_ = "";
  isKeepAlive_ = false;
  mmFile_ = nullptr;
  mmFileStat_ = {0};
}
HttpResponse::~HttpResponse() {
  UnmapFile();
}
void HttpResponse::Init(const std::string &srcDir, std::string path, bool isKeepAlive, int code) {
  assert(!srcDir.empty());
  if (mmFile_) {
    UnmapFile();
  }
  isKeepAlive_ = isKeepAlive;
  path_ = path;
  srcDir_ = srcDir;
  mmFile_ = nullptr;
  mmFileStat_ = {0};
}
void HttpResponse::MakeResponse(Buffer &buff) {
  /*判断请求资源文件*/
  /*获取文件属性，并存放到mmFileStat_中*/
  if (stat((srcDir_ + path_).data(), &mmFileStat_) < 0 || S_ISDIR(mmFileStat_.st_mode)) {
    /*获取文件属性，成功返回0|| 判断文件模式是否是目录*/
    code_ = 404;
  } else if (!(mmFileStat_.st_mode & S_IROTH)) {
    /*判断是否有读取权限S_IROTH*/
    code_ = 403;
  } else if (code_ == -1) {
    /*默认值则为资源存在，有读取权限，状态码200*/
    code_ = 200;
  }
  ErrorHtml_();
  AddStateLine_(buff);
  AddHeader_(buff);
  AddContent_(buff);
}
char *HttpResponse::File() {
  return mmFile_;
}
size_t HttpResponse::FileLen() const {
  return mmFileStat_.st_size;
}
void HttpResponse::ErrorHtml_() {
  if (CODE_PATH.count(code_) == 1) {
    path_ = CODE_PATH.find(code_)->second;
    stat((srcDir_ + path_).data(), &mmFileStat_);
  }
}
void HttpResponse::AddStateLine_(Buffer &buff) {
  std::string status;
  if (CODE_STATUS.count(code_) == 1) {
    status = CODE_STATUS.find(code_)->second;
  } else {
    code_ = 400;
    status = CODE_STATUS.find(code_)->second;
  }
  /*缓存中写入 响应状态行*/
  buff.Append("HTTP/1.1 " + std::to_string(code_) + " " + status + "\r\n");
}
void HttpResponse::AddHeader_(Buffer &buff) {
  buff.Append("Connection: ");
  if (isKeepAlive_) {
    buff.Append("keep-alive\r\n");
    buff.Append("keep-alive: max=6, timeout=120\r\n");
  } else {
    buff.Append("close\r\n");
  }
  buff.Append("Content-type: " + GetFileType_() + "\r\n");
}
void HttpResponse::AddContent_(Buffer &buff) {
  int srcFd = open((srcDir_ + path_).data(), O_RDONLY);
  if (srcFd < 0) {
    ErrorContent(buff, "File Not Found!");
    return;
  }
  /*将文件映射到内核中，提高文件的访问速度
   * MAP_PRIVATE 建立一个写入时拷贝的私有映射,即在内存映射区写入，不会影响到文件本身*/
  LOG_DEBUG("file path %s", (srcDir_ + path_).data())
  int *mmRet = (int *) mmap(nullptr, mmFileStat_.st_size, PROT_READ, MAP_PRIVATE, srcFd, 0);
  if (*mmRet == -1) {
    /*映射失败*/
    ErrorContent(buff, "File Not Found!");
    return;
  }
  mmFile_ = (char *) mmRet;
  close(srcFd);
  buff.Append("Content-length: " + std::to_string(mmFileStat_.st_size) + "\r\n\r\n");
}
void HttpResponse::UnmapFile() {
  if (mmFile_) {
    munmap(mmFile_, mmFileStat_.st_size);
    mmFile_ = nullptr;
  }
}
std::string HttpResponse::GetFileType_() {
  /*判断文件类型*/
  /*找到最有一个'.'出现的位置*/
  std::string::size_type idx = path_.find_last_of('.');
  if (idx == std::string::npos) {
    return "text/plain";
  }
  std::string suffix = path_.substr(idx);
  if (SUFFIX_TYPE.count(suffix) == 1) {
    return SUFFIX_TYPE.find(suffix)->second;
  }
  return "text/plain";
}
void HttpResponse::ErrorContent(Buffer &buff, const std::string &message) {
  std::string body;
  std::string status;
  body += "<html><title>Error</title>";
  body += "<body bgcolor=\"ffffff\">";
  if (CODE_STATUS.count(code_) == 1) {
    status = CODE_STATUS.find(code_)->second;
  } else {
    status = "Bad Request";
  }
  body += std::to_string(code_) + " : " + status + "\n";
  body += "<p>" + message + "</p>";
  body += "<hr><em>TinyWebServer</em></body></html>";

  buff.Append("Content-length: " + std::to_string(body.size()) + "\r\n\r\n");
  buff.Append(body);
}
