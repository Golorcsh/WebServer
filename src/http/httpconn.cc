//
// Created by chenshihao on 2021/12/11.
//

#include "httpconn.h"

const char *HttpConn::srcDir;
std::atomic<int> HttpConn::userCount;
bool HttpConn::isET;

HttpConn::HttpConn() {
  fd_ = -1;
  addr_ = {0};
  isClose_ = true;
}
HttpConn::~HttpConn() {
  Close();
}
void HttpConn::Init(int sockFd, const sockaddr_in &addr) {
  assert(sockFd > 0);
  userCount++;
  addr_ = addr;
  fd_ = sockFd;
  writeBuff_.RetrieveAll();
  readBuff_.RetrieveAll();
  isClose_ = false;
  LOG_INFO("Client[%d](%s:%d) in ,userCount:%d", fd_, GetIP(), GetPort(), (int) userCount)
}
void HttpConn::Close() {
  response_.UnmapFile();
  if (!isClose_) {
    isClose_ = true;
    userCount--;
    close(fd_);
    LOG_INFO("Client[%d](%s:%d) quit ,userCount:%d", fd_, GetIP(), GetPort(), (int) userCount)
  }
}
int HttpConn::GetFd() const {
  return fd_;
}
struct sockaddr_in HttpConn::GetAddr() const {
  return addr_;
}
const char *HttpConn::GetIP() const {
  return inet_ntoa(addr_.sin_addr);
}
int HttpConn::GetPort() const {
  return addr_.sin_port;
}
ssize_t HttpConn::Read(int *saveErrno) {
  /*从fd_中读取内容到readBuff中*/
  ssize_t len = -1;
  do {
    len = readBuff_.ReadFD(fd_, saveErrno);
    if (len <= 0) {
      break;
    }
  } while (isET);
  return len;
}
ssize_t HttpConn::Write(int *saveErrno) {
  /*将iov_内容写入到fd_中*/
  ssize_t len = -1;
  do {
    len = writev(fd_, iov_, iovCnt_);
    if (len <= 0) {
      *saveErrno = errno;
      break;
    }
    if (iov_[0].iov_len + iov_[1].iov_len == 0) { break; }/*没有数据，传输结束*/
    else if (static_cast<size_t>(len) > iov_[0].iov_len) {/*写入的数据大于第一个块，但还没完全写入*/
      iov_[1].iov_base = (uint8_t *) iov_[1].iov_base + (len - iov_[0].iov_len);/*将指针移动到还未写的地方*/
      iov_[1].iov_len -= (len - iov_[0].iov_len);/*更新剩余的长度*/
      if (iov_[0].iov_len) {
        writeBuff_.RetrieveAll();
        iov_[0].iov_len = 0;
      }
    } else {/*若写入数据量小于iov_[0]说明数据值使用了第一块，并且写入的数据小于第一块，第一块还有没写入更新写入*/
      iov_[0].iov_base = (uint8_t *) iov_[0].iov_base + len;
      iov_[0].iov_len -= len;
      writeBuff_.Retrieve(len);
    }
  } while (isET || ToWriteBytes() > 10240);
  return len;
}
bool HttpConn::Process() {
  /*分析请求，产生返回内容*/
  request_.Init();
  if (readBuff_.ReadableBytes() <= 0) {
    return false;
  } else if (request_.Parse(readBuff_)) {
    LOG_DEBUG("%s", request_.Path().c_str());
    response_.Init(srcDir, request_.Path(), request_.IsKeepAlive(), 200);
  } else {
    response_.Init(srcDir, request_.Path(), false, 400);
  }

  /*将产生返回内容写入到writeBuff中*/
  response_.MakeResponse(writeBuff_);
  /*将writeBuff的内容存储到iov_中,以便写入*/
  /*响应头*/
  iov_[0].iov_base = const_cast<char *>(writeBuff_.Peek());
  iov_[0].iov_len = writeBuff_.ReadableBytes();
  iovCnt_ = 1;

  /*文件*/
  if (response_.FileLen() > 0 && response_.File()) {
    iov_[1].iov_base = response_.File();
    iov_[1].iov_len = response_.FileLen();
    iovCnt_ = 2;
  }
  LOG_DEBUG("filesize:%d ,%d to %d", response_.FileLen(), iovCnt_, ToWriteBytes());
  return true;
}