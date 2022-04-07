//
// Created by chenshihao on 2021/12/4.
//

#include "buffer.h"
Buffer::Buffer(int init_buffer_size) : buffer_(init_buffer_size), readPos_(0), writePos_(0) {

}
/*buffer_中可以读入字节数*/
size_t Buffer::ReadableBytes() const {
  return writePos_ - readPos_;
}
/*buffer_中可写字节数*/
size_t Buffer::WritableBytes() const {
  return buffer_.size() - writePos_;
}
/*可预留的字节数*/
size_t Buffer::PrependableBytes() const {
  return readPos_;
}
/*返回当前读指针的位置*/
const char *Buffer::Peek() const {
  return BeginPtr_() + readPos_;
}
/*从当前读指针的位置，移动len长度，表示不要len长度的数据*/
void Buffer::Retrieve(size_t len) {
  assert(len <= ReadableBytes());
  readPos_ += len;
}
/*从当前都指针位置，移动end位置*/
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
/*返回开始写入的位置，const版本*/
const char *Buffer::BeginWriteConst() const {
  return BeginPtr_() + writePos_;
}
/*返回开始写入的位置，非const版本*/
char *Buffer::BeginWrite() {
  return BeginPtr_() + writePos_;
}
/*从当前写指针位置移动len长度，表示已经写入len长度的内容*/
void Buffer::HasWritten(size_t len) {
  writePos_ += len;
}
/*往buffer中append数据，接受string类型，然后调用重载Append插入函数*/
void Buffer::Append(const std::string &str) {
  Append(str.data(), str.length());
}
/*重载版本Append函数，将void类型转为char*类型，调用重载版本Append函数*/
void Buffer::Append(const void *data, size_t len) {
  assert(data);
  Append(static_cast<const char *>(data), len);
}
/*实际执行插入的Append函数*/
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
/*确保有足够的空间可以写入*/
void Buffer::EnsureWriteable(size_t len) {
  if (WritableBytes() < len) {/*空间不够就重新分配空间*/
    MakeSpace_(len);
  }
  assert(WritableBytes() >= len);
}
//从文件描述附中读取数据写入到buffer中
ssize_t Buffer::ReadFd(int fd, int *Errno) {
  /*从fd中读取数据，已块的方式读取，分为两个块*/
  char buff[65535];
  struct iovec iov[2];
  const size_t writable = WritableBytes();
  /* 分散读， 保证数据全部读完 */
  iov[0].iov_base = BeginPtr_() + writePos_;
  iov[0].iov_len = writable;
  iov[1].iov_base = buff;
  iov[1].iov_len = sizeof(buff);
  const ssize_t len = readv(fd, iov, 2);
  if (len < 0) {
    *Errno = errno;
  } else if (static_cast<size_t>(len) <= writable) {/*buffer_足够的，则直接写入*/
    writePos_ += len;
  } else {/*buffer_不够大，则将剩余的通过Append方法写入（append方法会自动扩容）*/
    writePos_ = buffer_.size();
    Append(buff, len - writable);
  }
  return len;
}
//从buffer中读取数据然后向文件描述符中写入
ssize_t Buffer::WriteFd(int fd, int *Errno) {
  /*向fd中写入数据*/
  size_t readSize = ReadableBytes();
  ssize_t len = write(fd, Peek(), readSize);
  if (len < 0) {
    *Errno = errno;
    return len;
  }
  readPos_ += len;
  return len;
}
char *Buffer::BeginPtr_() {
  return &*buffer_.begin();
}
const char *Buffer::BeginPtr_() const {
  return &*buffer_.begin();
}
void Buffer::MakeSpace_(size_t len) {
  //还可以写入的空间和已经读取的空间的总共空间还是小于len,则需要分配空间
  if (WritableBytes() + PrependableBytes() < len) {
    buffer_.resize(writePos_ + len + 1);
  } else {
    /*将还未读取的数据buffer[readOps,writeOps]，移动到buffer的开头，更新读写位置*/
    size_t readable = ReadableBytes();
    std::copy(BeginPtr_() + readPos_, BeginPtr_() + writePos_, BeginPtr_());
    readPos_ = 0;
    writePos_ = readPos_ + readable;
    assert(readable == ReadableBytes());
  }
}
