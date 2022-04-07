//
// Created by chenshihao on 2021/12/7.
//
#include "heaptimer.h"

void HeapTimer::siftup_(size_t i) {
/*堆的上浮操作,用于插入节点后，判断当前节点是否需要上浮*/
  assert(i >= 0 && i < heap_.size());
  size_t father = (i - 1) / 2;/*获得父节点*/
  while (father >= 0) {
    if (heap_[father] < heap_[i])break;
    SwapNode_(i, father);/*交换节点*/
    i = father;/*更新*/
    father = (i - 1) / 2;/*更新新的父节点*/
  }
}
void HeapTimer::SwapNode_(size_t i, size_t j) {
  assert(i >= 0 && i < heap_.size());
  assert(j >= 0 && j < heap_.size());
  std::swap(heap_[i], heap_[j]);
  /*更新索引*/
  ref_[heap_[i].id] = i;
  ref_[heap_[j].id] = j;
}
bool HeapTimer::siftdown_(size_t index, size_t n) {
/*堆的下浮操作,用于修改节点后，判断当前节点是否需要下浮*/
  assert(index >= 0 && index < heap_.size());
  assert(n >= 0 && n <= heap_.size());/*小于等于考虑到删除元素时需要缩小大小*/
  size_t i = index;
  size_t child = i * 2 + 1;/*左节点*/
  while (child < n) {
    /*如果有子树小于左子树，则将j移动到右子树*/
    if (child + 1 < n && heap_[child + 1] < heap_[child])child++;
    /*判断当前节点是否大于子树节点,若小于直接推出,否则进行交换，然后递归往下*/
    if (heap_[i] < heap_[child]) break;
    SwapNode_(i, child);
    i = child;/*更新当前节点坐标*/
    child = i * 2 + 1;/*获得新的左子树节点*/
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
    siftup_(i);/*上浮*/
  } else {
    /*已经有的节点，则修改*/
    i = ref_[id];
    heap_[i].expires = Clock::now() + MS(timeout);
    heap_[i].cb = cb;
    /*下浮操作，若没有下浮则上浮*/
    if (!siftdown_(i, heap_.size())) {
      siftup_(i);
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
  del_(i);
}
void HeapTimer::del_(size_t index) {
  /*删除指定位置的节点*/
  assert(!heap_.empty() && index >= 0 && index < heap_.size());
  /*将要删除的节点换到堆尾部，然后调整*/
  size_t i = index;
  size_t n = heap_.size() - 1;
  assert(i <= n);
  if (i < n) {
    SwapNode_(i, n);
    if (!siftdown_(i, n)) {
      siftup_(i);
    }
  }
  /*删除索引和移除元素*/
  ref_.erase(heap_.back().id);
  heap_.pop_back();
}
void HeapTimer::adjust(int id, int timeout) {
  /*调整指定id的节点*/
  assert(!heap_.empty() && ref_.count(id) != 0);
  heap_[ref_[id]].expires = Clock::now() + MS(timeout);
  /*由于是新设置的时间，只会更大，不会更小，因此只需下浮*/
  siftdown_(ref_[id], heap_.size());
}


void HeapTimer::tick() {
/*删除过期节点*/
  if (heap_.empty()){
    return;
  }
  while (!heap_.empty()) {
    auto node = heap_.front();
    if (std::chrono::duration_cast<MS>(node.expires - Clock::now()).count() > 0)
      break;
    node.cb();/*超时执行回调函数*/
    pop();
  }
}
void HeapTimer::pop() {
    assert(!heap_.empty());
  del_(0);
}
void HeapTimer::clear() {
  ref_.clear();
  heap_.clear();
}

int HeapTimer::GetNextTick() {
  tick();
  size_t res = -1;
  if (!heap_.empty()) {
    res = std::chrono::duration_cast<MS>(heap_.front().expires - Clock::now()).count();
    if (res < 0){res = 0;}
  }
  return res;
}

