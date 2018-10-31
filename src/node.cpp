#include <sys/mman.h>
#include <chrono>
#include "node.h"
#ifdef __APPLE__
#include <sys/socket.h>
#else
#include <signal.h>
#endif

#define MIN_BUFSIZE 4096

using namespace std;

namespace rokid {
namespace lizard {

// ==================Buffer====================
void Buffer::set_data(void* p, uint32_t size, uint32_t b, uint32_t e) {
  datap = (int8_t*)p;
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
    memmove(datap, datap + begin, sz);
  }
  begin = 0;
  end = begin + sz;
}

void Buffer::clear() {
  begin = end = 0;
}

void Buffer::move(Buffer& src) {
  set_data(src.datap, src.capacity, src.begin, src.end);
  src.clear();
}

void Buffer::assign(Buffer& src) {
  set_data(src.datap, src.capacity, src.begin, src.end);
}

bool Buffer::append(const void* data, uint32_t size) {
  if (end + size > capacity)
    return false;
  memcpy(datap + end, data, size);
  end += size;
  return true;
}

MmapBuffer::MmapBuffer(uint32_t size) {
  if (size < MIN_BUFSIZE)
    size = MIN_BUFSIZE;
  datap = (int8_t*)mmap(NULL, size, PROT_READ | PROT_WRITE,
      MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if (datap) {
    capacity = size;
  }
}

MmapBuffer::~MmapBuffer() {
  if (datap) {
    munmap(datap, capacity);
  }
}

// ==================Node====================
bool Node::init(Uri& uri, NodeError* err, uint32_t argc, void** args) {
  void* targ = argc ? *args : nullptr;
  uint32_t sargc = argc ? argc - 1 : 0;
  void** sargs = argc ? args + 1 : nullptr;
  if (super_node && !super_node->init(uri, err, sargc, sargs)) {
    return false;
  }
  if (!on_init(uri, err, targ)) {
    if (err) {
      err->node = this;
    }
    return false;
  }
  return true;
}

bool Node::write(Buffer& in, NodeError* err, uint32_t argc, void** args) {
  Buffer result;
  int32_t r;
  void* targ = argc ? *args : nullptr;
  uint32_t sargc = argc ? argc - 1 : 0;
  void** sargs = argc ? args + 1 : nullptr;
  lock_guard<mutex> locker(write_mutex);

  while (true) {
    r = on_write(in, result, err, targ);
    if (r < 0) {
      if (err) {
        err->node = this;
      }
      return false;
    }
    if (super_node) {
      if (!super_node->write(result, err, sargc, sargs)) {
        return false;
      }
    }
    if (r == 0) {
      break;
    }
  }
  return true;
}

bool Node::read(Buffer& out, NodeError* err, uint32_t argc,
    void** out_args) {
  int32_t r;
  void** targ = argc ? out_args : nullptr;
  uint32_t sargc = argc ? argc - 1 : 0;
  void** sargs = argc ? out_args + 1 : nullptr;

  while (true) {
    r = on_read(out, err, targ);
    if (r < 0) {
      if (err) {
        err->node = this;
        return false;
      }
    }
    if (r && super_node) {
      if (read_buffer == nullptr || read_buffer->total_space() == 0) {
        if (err) {
          err->node = this;
          err->code = -1;
          err->descript = "read buffer not initialize";
        }
        return false;
      }
      if (!super_node->read(*read_buffer, err, sargc, sargs)) {
        return false;
      }
    } else {
      break;
    }
  }
  return true;
}

void Node::close() {
  lock_guard<mutex> locker(write_mutex);
  on_close();
  if (super_node) {
    super_node->close();
  }
}

void Node::chain(Node* node) {
  super_node = node;
}

void ignore_sigpipe(int socket) {
#ifdef __APPLE__
  int option_value = 1; /* Set NOSIGPIPE to ON */
  setsockopt(socket, SOL_SOCKET, SO_NOSIGPIPE, &option_value,
        sizeof(option_value));
#else
  static bool ignored_sigpipe = false;
  if (!ignored_sigpipe) {
    struct sigaction act;
    int r;
    memset(&act, 0, sizeof(act));
    act.sa_handler = SIG_IGN;
    r = sigaction(SIGPIPE, &act, nullptr);
    if (r == 0)
      ignored_sigpipe = true;
  }
#endif // __APPLE__
}

} // namespace lizard
} // namespace rokid
