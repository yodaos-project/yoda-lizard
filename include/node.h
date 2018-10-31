#pragma once

#include <mutex>
#include "uri.h"

namespace rokid {
namespace lizard {

class Node;
typedef struct {
  Node* node;
  int32_t code;
  const char* descript;
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

class MmapBuffer : public Buffer {
public:
  MmapBuffer(uint32_t size);

  ~MmapBuffer();
};

class Node {
public:
  virtual ~Node() = default;

  bool init(rokid::Uri& uri, NodeError* err = nullptr, uint32_t argc = 0,
      void** args = nullptr);

  bool write(Buffer& in, NodeError* err = nullptr, uint32_t argc = 0,
      void** args = nullptr);

  bool read(Buffer& out, NodeError* err = nullptr, uint32_t argc = 0,
      void** out_args = nullptr);

  void close();

  void chain(Node* node);

  virtual const char* name() const = 0;

protected:
  virtual bool on_init(rokid::Uri& uri, NodeError* err, void* arg) = 0;

  virtual int32_t on_write(Buffer& in, Buffer& out, NodeError* err,
      void* arg) = 0;

  // return: 0  read success and complete
  //         1  read data not complete
  //         -1 read failed or data invalid
  virtual int32_t on_read(Buffer& out, NodeError* err, void** out_arg) = 0;

  virtual void on_close() = 0;

protected:
  Node* super_node = nullptr;
  Buffer* read_buffer = nullptr;

private:
  std::mutex write_mutex;
};

} // namespace lizard
} // namespace rokid
