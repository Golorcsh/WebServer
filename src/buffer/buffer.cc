//
// Created by chenshihao on 2021/12/4.
//

#include "buffer.h"
Buffer::Buffer(int init_buffer_size) : buffer_(init_buffer_size), readPos_(0), writePos_(0) {

}
/*buffer_中可写字节数*/
size_t Buffer::WriteableBytes() const {
  return writePos_ - readPos_;
}
/*buffer_中可以读入字节数*/
size_t Buffer::ReadableBytes() const {
  return buffer_.size() - writePos_;
}
/*可预留的字节数*/
size_t Buffer::PrependableBytes() const {
  return readPos_;
}
const char *Buffer::Peek() const {
  return BeginPtr() + readPos_;
}
void Buffer::Retrieve(size_t len) {
  assert(len <= ReadableBytes());
  readPos_ += len;
}
void Buffer::RetrieveUntil(const char *end) {
  assert(Peek() <= end);
  Retrieve(end - Peek());
}
/*将buffer中都给置空，读写置空*/
void Buffer::RetrieveAll() {
  bzero(&buffer_[0], buffer_.size());
  readPos_ = 0;
  writePos_ = 0;
}
/*将Peek位置开始的数据读取转换为string,清空buffer*/
std::string Buffer::RetrieveAllToStr() {
  std::string str(Peek(), ReadableBytes());
  RetrieveAll();
  return str;
}
const char *Buffer::BeginWriteConst() const {
  return BeginPtr() + writePos_;
}
char *Buffer::BeginWrite() {
  return BeginPtr() + writePos_;
}
void Buffer::HasWritten(size_t len) {
  writePos_ += len;
}
void Buffer::Append(const std::string &str) {
  Append(str.data(), str.length());
}
void Buffer::Append(const void *data, size_t len) {
  assert(data);
  Append(static_cast<const char *>(data), len);
}
void Buffer::Append(const char *str, size_t len) {
  assert(str);
  EnsureWriteable(len);
  /*深拷贝，将内容拷贝到buffer中*/
  std::copy(str, str + len, BeginWrite());
  HasWritten(len);
}
void Buffer::Append(const Buffer &buff) {
  Append(buff.Peek(), buff.ReadableBytes());
}
void Buffer::EnsureWriteable(size_t len) {
  /*确保有足够的空间可以写入*/
  if (WriteableBytes() < len) {/*空间不够就重新分配空间*/
    MakeSpace_(len);
  }
  assert(WriteableBytes() >= len);
}
ssize_t Buffer::ReadFD(int fd, int *Errno) {
  /*从fd中读取数据，已块的方式读取，分为两个块*/
  char buff[65535];
  struct iovec iov[2];
  const size_t writeable = WriteableBytes();
  /*分散读，保证数据全部读取完*/
  iov[0].iov_base = BeginPtr() + writePos_;
  iov[0].iov_len = writeable;
  iov[1].iov_base = buff;
  iov[1].iov_len = sizeof(buff);
  const ssize_t len = readv(fd, iov, 2);
  if (len < 0) {
    *Errno = errno;
  } else if (static_cast<size_t>(len) <= writeable) {/*buffer_足够的，则直接写入*/
    writePos_ += len;
  } else {/*buffer_不够大，则将剩余的通过Append方法写入（append方法会自动扩容）*/
    writePos_ = buffer_.size();
    Append(buff, len - writeable);
  }
  return len;
}
ssize_t Buffer::WriteFD(int fd, int *Erron) {
  /*向fd中写入数据*/
  size_t readable = ReadableBytes();
  ssize_t len = write(fd, Peek(), readable);
  if (len < 0) {
    *Erron = errno;
    return len;
  }
  readPos_ += len;
  return len;
}
char *Buffer::BeginPtr() {
  return &*buffer_.begin();
}
const char *Buffer::BeginPtr() const {
  return &*buffer_.begin();
}
void Buffer::MakeSpace_(size_t len) {
  if (WriteableBytes() + PrependableBytes() < len) {
    buffer_.resize(writePos_ + len + 1);
  } else {
    /*将还未读取的数据buffer[readOps,writeOps]，移动到buffer的开头，更新读写位置*/
    size_t readable = ReadableBytes();
    std::copy(BeginPtr() + readPos_, BeginPtr() + writePos_, BeginPtr());
    readPos_ = 0;
    writePos_ = readPos_ + readable;
    assert(readable == ReadableBytes());
  }
}
