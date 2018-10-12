#pragma once

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

  Buffer(void* p, uint32_t size) : data((int8_t*)p), capacity(size) {
  }

  void set_data(void* p, uint32_t size, uint32_t b, uint32_t e);

  void obtain(uint32_t size);

  void consume(uint32_t size);

  void shift();

  void clear();

  void move(Buffer& src);

  bool append(const void* data, uint32_t size);

  bool empty() const { return end - begin == 0; }

  uint32_t size() const { return end - begin; }

public:
  int8_t* data = nullptr;
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

  bool init(rokid::Uri& uri, NodeError* err = nullptr);

  bool write(Buffer& in, NodeError* err = nullptr);

  bool read(Buffer& out, NodeError* err = nullptr, void** extra = nullptr);

  void close();

  void chain(Node* node);

  virtual const char* name() const = 0;

protected:
  virtual bool on_init(rokid::Uri& uri, NodeError* err) = 0;

  virtual int32_t on_write(Buffer& in, Buffer& out, NodeError* err) = 0;

  // return: 0  read success and complete
  //         1  read data not complete
  //         -1 read failed or data invalid
  virtual int32_t on_read(Buffer& out, NodeError* err, void* super_extra,
      void** extra) = 0;

  virtual void on_close() = 0;

protected:
  Node* super_node = nullptr;
  Buffer* read_buffer = nullptr;
  void* super_extra = nullptr;
};

} // namespace lizard
} // namespace rokid
