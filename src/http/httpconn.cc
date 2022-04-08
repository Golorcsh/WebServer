//
// Created by chenshihao on 2021/12/11.
//

#include "httpconn.h"

const char *HttpConn::src_dir_;
std::atomic<int> HttpConn::user_count_;
bool HttpConn::is_et_;

HttpConn::HttpConn() {
  fd_ = -1;
  addr_ = {0};
  is_close_ = true;
}
HttpConn::~HttpConn() {
  Close();
}
void HttpConn::Init(int sockFd, const sockaddr_in &addr) {
  assert(sockFd > 0);
  user_count_++;
  addr_ = addr;
  fd_ = sockFd;
  write_buff_.RetrieveAll();
  read_buff_.RetrieveAll();
  is_close_ = false;
  LOG_INFO("Client[%d](%s:%d) in ,user_count_:%d", fd_, GetIP(), GetPort(), (int) user_count_);
}
void HttpConn::Close() {
  response_.UnmapFile();
  if (is_close_ == false) {
    is_close_ = true;
    user_count_--;
    close(fd_);
    LOG_INFO("Client[%d](%s:%d) quit, UserCount:%d", fd_, GetIP(), GetPort(), (int) user_count_);
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
    len = read_buff_.ReadFd(fd_, saveErrno);
    if (len <= 0) {
      break;
    }
  } while (is_et_);
  return len;
}
ssize_t HttpConn::Write(int *saveErrno) {
  /*将iov_内容写入到fd_中*/
  ssize_t len = -1;
  do {
    len = writev(fd_, iov_, iov_cnt_);
    if (len <= 0) {
      *saveErrno = errno;
      break;
    }
    if (iov_[0].iov_len + iov_[1].iov_len == 0) { break; } /* 传输结束 */
    else if (static_cast<size_t>(len) > iov_[0].iov_len) {
      iov_[1].iov_base = (uint8_t *) iov_[1].iov_base + (len - iov_[0].iov_len);
      iov_[1].iov_len -= (len - iov_[0].iov_len);
      if (iov_[0].iov_len) {
        write_buff_.RetrieveAll();
        iov_[0].iov_len = 0;
      }
    } else {/*若写入数据量小于iov_[0]说明数据值使用了第一块，并且写入的数据小于第一块，第一块还有没写入更新写入*/
      iov_[0].iov_base = (uint8_t *) iov_[0].iov_base + len;
      iov_[0].iov_len -= len;
      write_buff_.Retrieve(len);
    }
  } while (is_et_ || ToWriteBytes() > 10240);
  return len;
}
bool HttpConn::Process() {
  /*分析请求，产生返回内容*/
  request_.Init();
  if (read_buff_.ReadableBytes() <= 0) {
    return false;
  } else if (request_.Parse(read_buff_)) {
    LOG_DEBUG("%s", request_.Path().c_str());
    response_.Init(src_dir_, request_.Path(), request_.IsKeepAlive(), 200);
  } else {
    response_.Init(src_dir_, request_.Path(), false, 400);
  }

  /*将产生返回内容写入到writeBuff中*/
  response_.MakeResponse(write_buff_);
  /*将writeBuff的内容存储到iov_中,以便写入*/
  /*响应头*/
  iov_[0].iov_base = const_cast<char *>(write_buff_.Peek());
  iov_[0].iov_len = write_buff_.ReadableBytes();
  iov_cnt_ = 1;

  /*文件*/
  if (response_.FileLen() > 0 && response_.File()) {
    iov_[1].iov_base = response_.File();
    iov_[1].iov_len = response_.FileLen();
    iov_cnt_ = 2;
  }
  LOG_DEBUG("filesize:%d ,%d to %d", response_.FileLen(), iov_cnt_, ToWriteBytes());
  return true;
}