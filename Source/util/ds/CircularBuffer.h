// Copyright (c) 2018-2021 Marco Wang <m.aesophor@gmail.com>. All rights reserved.
#ifndef VIGILANTE_CIRCULAR_BUFFER_H_
#define VIGILANTE_CIRCULAR_BUFFER_H_

#include <memory>
#include <string>

namespace vigilante {

template <typename T>
class CircularBuffer {
  static inline constexpr int kDefaultCapacity = 32;

 public:
  explicit CircularBuffer(int capacity = kDefaultCapacity)
    : _data(std::make_unique<T[]>(capacity)),
      _head(),
      _tail(),
      _size(),
      _capacity(capacity) {}
  virtual ~CircularBuffer() = default;
  T& operator[] (size_t i) { return _data[i]; }

  void push(T val);
  void pop();
  void clear();

  inline size_t size() const { return _size; }
  inline size_t capacity() const { return _capacity; }
  inline bool empty() const { return _tail == _head; }
  inline bool full() const { return (_tail + 1) % (int) _capacity == _head; }
  inline T front() const { return _data[_head]; }
  inline T back() const { return _data[(_tail - 1 < 0) ? _capacity - 1 : _tail - 1]; }

 protected:
  std::unique_ptr<T[]> _data;
  int _head;
  int _tail;
  size_t _size;
  const size_t _capacity;
};

template <typename T>
void CircularBuffer<T>::push(T val) {
  _data[_tail] = val;
  
  if (full()) {
    _head = (_head + 1) % _capacity;
  }
  _tail = (_tail + 1) % _capacity;

  if (_size < _capacity) {
    _size++;
  }
}

template <typename T>
void CircularBuffer<T>::pop() {
  _head = (_head + 1) % _capacity;

  if (_size > 0) {
    _size--;
  }
}

template <typename T>
void CircularBuffer<T>::clear() {
  _tail = 0;
  _head = 0;
}

}  // namespace vigilante

#endif  // VIGILANTE_CIRCULAR_BUFFER_H_
