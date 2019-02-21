#pragma once

#include <list>
#include <string>
#include "uri.h"

namespace rokid {
namespace lizard {

class Node;
typedef struct {
  Node* node;
  int32_t code;
  std::string desc;
} NodeError;

class Buffer {
public:
  Buffer() {}

  Buffer(void* p, uint32_t size) : datap((int8_t*)p), capacity(size) {
  }

  void set_data(void* p, uint32_t size, uint32_t b, uint32_t e);

  void obtain(uint32_t size);

  void consume(uint32_t size);

  void shift();

  void clear();

  void move(Buffer& src);

  void assign(Buffer& src);

  bool append(const void* data, uint32_t size);

  bool empty() const { return end - begin == 0; }

  uint32_t size() const { return end - begin; }

  void* data_begin() { return datap + begin; }

  void* data_end() { return datap + end; }

  uint32_t remain_space() const { return capacity - end; }

  uint32_t total_space() const { return capacity; }

protected:
  int8_t* datap = nullptr;
  uint32_t begin = 0;
  uint32_t end = 0;
  uint32_t capacity = 0;
};

/**
class MmapBuffer : public Buffer {
public:
  MmapBuffer(uint32_t size);

  ~MmapBuffer();
};
*/

template <typename T>
class NodeArgs {
public:
  void push(T *v) {
    queue.push_back(v);
  }

  T *front() {
    if (queue.empty())
      return nullptr;
    return queue.front();
  }

  T *back() {
    if (queue.empty())
      return nullptr;
    return queue.back();
  }

  T *pop_front() {
    if (queue.empty())
      return nullptr;
    T *v = queue.front();
    queue.pop_front();
    return v;
  }

  T *pop_back() {
    if (queue.empty())
      return nullptr;
    T *v = queue.back();
    queue.pop_back();
    return v;
  }

  void clear() {
    queue.clear();
  }

private:
  std::list<T*> queue;
};

class Node {
public:
  virtual ~Node() = default;

  void set_read_buffers(NodeArgs<Buffer> *bufs);

  void set_write_buffers(NodeArgs<Buffer> *bufs);

  inline void set_read_buffer(Buffer *buf) { read_buffer = buf; }

  inline void set_write_buffer(Buffer *buf) { write_buffer = buf; }

  bool init(const rokid::Uri& uri, NodeArgs<void> *args = nullptr);

  bool write(Buffer *in, NodeArgs<void> *args = nullptr);

  bool read(Buffer *out, NodeArgs<void> *args = nullptr);

  void close();

  void chain(Node* node);

  inline const NodeError *get_error() const { return &err_info; }

  virtual const char* name() const = 0;

protected:
  virtual bool on_init(const rokid::Uri& uri, void *arg) = 0;

  virtual int32_t on_write(Buffer *in, Buffer *out, void* arg) = 0;

  // return: 0  read success and complete
  //         1  read data not complete
  //         -1 read failed or data invalid
  virtual int32_t on_read(Buffer *out, Buffer *in, void *arg) = 0;

  virtual void on_close() = 0;

  void clear_node_error();

protected:
  Node* super_node = nullptr;
  Buffer *read_buffer = nullptr;
  Buffer *write_buffer = nullptr;
  static thread_local NodeError err_info;
};

} // namespace lizard
} // namespace rokid
