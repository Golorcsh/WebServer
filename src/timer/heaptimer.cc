//
// Created by chenshihao on 2021/12/7.
//
#include "heaptimer.h"

/*!
 * @brief 堆的上浮操作,用于插入节点后，判断当前节点是否需要上浮
 * @param i
 */
void HeapTimer::Siftup(size_t i) {
  assert(i >= 0 && i < heap_.size());
  size_t father = (i - 1) / 2; /*获得父节点*/
  while (father >= 0) {
    if (heap_[father] < heap_[i])
      break;
    SwapNode(i, father);  /*交换节点*/
    i = father;           /*更新*/
    father = (i - 1) / 2; /*更新新的父节点*/
  }
}
/*!
 * @brief 交换两个节点，更新索引
 * @param i
 * @param j
 */
void HeapTimer::SwapNode(size_t i, size_t j) {
  assert(i >= 0 && i < heap_.size());
  assert(j >= 0 && j < heap_.size());
  std::swap(heap_[i], heap_[j]);
  /*更新索引*/
  ref_[heap_[i].id] = i;
  ref_[heap_[j].id] = j;
}
/*!
 * @brief 堆的下浮操作,用于修改节点后，判断当前节点是否需要下浮
 * @param index
 * @param n
 * @return
 */
bool HeapTimer::Siftdown(size_t index, size_t n) {
  /*
  assert(index >= 0 && index < heap_.size());
  assert(n >= 0 && n <= heap_.size()); /*小于等于考虑到删除元素时需要缩小大小*/
  size_t i = index;
  size_t child = i * 2 + 1; /*左节点*/
  while (child < n) {
    /*如果有子树小于左子树，则将j移动到右子树*/
    if (child + 1 < n && heap_[child + 1] < heap_[child])
      child++;
    /*判断当前节点是否大于子树节点,若小于直接推出,否则进行交换，然后递归往下*/
    if (heap_[i] < heap_[child])
      break;
    SwapNode(i, child);
    i = child;         /*更新当前节点坐标*/
    child = i * 2 + 1; /*获得新的左子树节点*/
  }
  return i > index;
}
void HeapTimer::add(int id, int timeout, const TimeoutCallBack &cb) {
  assert(id >= 0);
  size_t i;
  if (ref_.count(id) == 0) {
    /*新节点不存在，堆尾部插入，调整堆*/
    i = heap_.size();
    ref_[id] = i;
    /*插入新的timer*/
    heap_.push_back({id, Clock::now() + MS(timeout), cb});
    Siftup(i); /*上浮*/
  } else {
    /*已经有的节点，则修改*/
    i = ref_[id];
    heap_[i].expires = Clock::now() + MS(timeout);
    heap_[i].cb = cb;
    /*下浮操作，若没有下浮则上浮*/
    if (!Siftdown(i, heap_.size())) {
      Siftup(i);
    }
  }
}
void HeapTimer::doWork(int id) {
  /*执行回调函数*/
  if (heap_.empty() || ref_.count(id) == 0) {
    return;
  }
  size_t i = ref_[id];
  TimerNode node = heap_[i];
  node.cb();
  Del(i);
}
/*!
 * @brief 删除结点
 * @param i
 */
void HeapTimer::Del(size_t i) {
  /*删除指定位置的节点*/
  assert(!heap_.empty() && i >= 0 && i < heap_.size());
  /*将要删除的节点换到堆尾部，然后调整*/
  size_t j = i;
  size_t n = heap_.size() - 1;
  assert(j <= n);
  if (j < n) {
    SwapNode(j, n);
    if (!Siftdown(j, n)) {
      Siftup(j);
    }
  }
  /*删除索引和移除元素*/
  ref_.erase(heap_.back().id);
  heap_.pop_back();
}
/*!
 * @brief 调整指定id的节点
 * @param id
 * @param timeout
 */
void HeapTimer::Adjust(int id, int timeout) {
  assert(!heap_.empty() && ref_.count(id) != 0);
  heap_[ref_[id]].expires = Clock::now() + MS(timeout);
  /*由于是新设置的时间，只会更大，不会更小，因此只需下浮*/
  Siftdown(ref_[id], heap_.size());
}

/*!
 * @brief 删除过期节点
 */
void HeapTimer::Tick() {
  if (heap_.empty()) {
    return;
  }
  while (!heap_.empty()) {
    auto node = heap_.front();
    if (std::chrono::duration_cast<MS>(node.expires - Clock::now()).count() > 0)
      break;
    node.cb(); /*超时执行回调函数*/
    Pop();
  }
}
/*!
 * @brief 出堆
 */
void HeapTimer::Pop() {
  assert(!heap_.empty());
  Del(0);
}
/*!
 * @brief 清除堆
 */
void HeapTimer::clear() {
  ref_.clear();
  heap_.clear();
}
/*!
 * @brief获得过期时间ms
 * @return
 */
int HeapTimer::GetNextTick() {
  Tick();
  size_t res = -1;
  if (!heap_.empty()) {
    res = std::chrono::duration_cast<MS>(heap_.front().expires - Clock::now())
              .count();
    if (res < 0) {
      res = 0;
    }
  }
  return (int)res;
}
