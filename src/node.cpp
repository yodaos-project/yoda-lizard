#include <sys/mman.h>
#include "node.h"

#define MIN_BUFSIZE 4096

namespace rokid {
namespace lizard {

// ==================Buffer====================
void Buffer::set_data(void* p, uint32_t size, uint32_t b, uint32_t e) {
  data = (int8_t*)p;
  capacity = size;
  begin = b;
  end = e;
}

void Buffer::obtain(uint32_t size) {
  end += size;
  if (end > capacity)
    end = capacity;
}

void Buffer::consume(uint32_t size) {
  begin += size;
  if (begin > end)
    begin = end;
  if (begin == end)
    clear();
}

void Buffer::shift() {
  if (begin == 0)
    return;
  uint32_t sz = end - begin;
  if (sz) {
    memmove(data, data + begin, sz);
  }
  begin = 0;
  end = begin + sz;
}

void Buffer::clear() {
  begin = end = 0;
}

void Buffer::move(Buffer& src) {
  set_data(src.data, src.capacity, src.begin, src.end);
  src.clear();
}

bool Buffer::append(const void* data, uint32_t size) {
  if (end + size > capacity)
    return false;
  memcpy(this->data + end, data, size);
  end += size;
  return true;
}

MmapBuffer::MmapBuffer(uint32_t size) {
  if (size < MIN_BUFSIZE)
    size = MIN_BUFSIZE;
  data = (int8_t*)mmap(NULL, size, PROT_READ | PROT_WRITE,
      MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if (data) {
    capacity = size;
  }
}

MmapBuffer::~MmapBuffer() {
  if (data) {
    munmap(data, capacity);
  }
}

// ==================Node====================
bool Node::init(Uri& uri, NodeError* err) {
  if (super_node && !super_node->init(uri, err)) {
    if (err) {
      err->node = this;
    }
    return false;
  }
  if (!on_init(uri, err)) {
    if (err) {
      err->node = this;
    }
    return false;
  }
  return true;
}

bool Node::write(Buffer& in, NodeError* err) {
  Buffer result;
  int32_t r;

  while (true) {
    r = on_write(in, result, err);
    if (r < 0) {
      if (err) {
        err->node = this;
      }
      return false;
    }
    if (super_node) {
      if (!super_node->write(result, err)) {
        return false;
      }
    }
    if (r == 0) {
      break;
    }
  }
  return true;
}

bool Node::read(Buffer& out, NodeError* err, void** extra) {
  int32_t r;
  while (true) {
    r = on_read(out, err, super_extra, extra);
    if (r < 0) {
      if (err) {
        err->node = this;
        return false;
      }
    }
    if (r && super_node) {
      if (read_buffer == nullptr || read_buffer->capacity == 0) {
        if (err) {
          err->node = this;
          err->code = -1;
          err->descript = "read buffer not initialize";
        }
        return false;
      }
      if (!super_node->read(*read_buffer, err, &super_extra)) {
        return false;
      }
    } else {
      break;
    }
  }
  return true;
}

void Node::close() {
  on_close();
  if (super_node) {
    super_node->close();
  }
}

void Node::chain(Node* node) {
  super_node = node;
}

} // namespace lizard
} // namespace rokid
