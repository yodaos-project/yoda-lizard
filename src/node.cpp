#include <sys/mman.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <errno.h>
#include <chrono>
#include "node.h"
#include "common.h"
#include "rlog.h"
#ifdef __APPLE__
#include <sys/socket.h>
#else
#include <signal.h>
#endif

#define MIN_BUFSIZE 4096

using namespace std;

namespace rokid {
namespace lizard {

thread_local NodeError Node::err_info;

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

/**
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
*/

// ==================Node====================
void Node::set_read_buffers(NodeArgs<Buffer> *bufs) {
  if (bufs == nullptr)
    return;
  read_buffer = bufs->get();
  if (super_node)
    super_node->set_read_buffers(bufs);
}

void Node::set_write_buffers(NodeArgs<Buffer> *bufs) {
  if (bufs == nullptr)
    return;
  write_buffer = bufs->get();
  if (super_node)
    super_node->set_write_buffers(bufs);
}

bool Node::init(const Uri& uri, NodeArgs<void> *args) {
  void* targ = args ? args->get() : nullptr;
  if (super_node && !super_node->init(uri, args)) {
    return false;
  }
  if (!on_init(uri, targ)) {
    return false;
  }
  clear_node_error();
  return true;
}

bool Node::write(Buffer *in, NodeArgs<void> *args) {
  uint32_t argsIndex{0};
  void* targ = args ? args->get(&argsIndex) : nullptr;
  bool ret{true};

  while (true) {
    auto r = on_write(in, write_buffer, targ);
    if (r < 0) {
      ret = false;
      goto exit;
    }
    if (super_node) {
      if (!super_node->write(write_buffer, args)) {
        ret = false;
        goto exit;
      }
    }
    if (r == 0) {
      break;
    }
  }

exit:
  if (args)
    args->restore(argsIndex);
  if (ret)
    clear_node_error();
  return ret;
}

bool Node::read(Buffer *out, NodeArgs<void> *args) {
  uint32_t argsIndex{0};
  void* targ = args ? args->get(&argsIndex) : nullptr;
  bool ret{true};

  while (true) {
    auto r = on_read(out, read_buffer, targ);
    if (r < 0) {
      ret = false;
      goto exit;
    }
    if (r && super_node) {
      if (!super_node->read(read_buffer, args)) {
        ret = false;
        goto exit;
      }
    } else {
      break;
    }
  }

exit:
  if (args)
    args->restore(argsIndex);
  if (ret)
    clear_node_error();
  return ret;
}

void Node::close() {
  clear_node_error();
  on_close();
  if (super_node) {
    super_node->close();
  }
  if (read_buffer)
    read_buffer->clear();
  if (write_buffer)
    write_buffer->clear();
}

void Node::chain(Node* node) {
  super_node = node;
}

void Node::clear_node_error() {
  err_info.node = nullptr;
  err_info.code = 0;
  err_info.desc.clear();
}

void set_rw_timeout(int socket, int32_t tm, bool rd) {
  struct timeval tv;
  if (tm > 0) {
    tv.tv_sec = tm / 1000;
    tv.tv_usec = (tm % 1000) * 1000;
  } else {
    tv.tv_sec = 0;
    tv.tv_usec = 0;
  }
  int opt = rd ? SO_RCVTIMEO : SO_SNDTIMEO;
  if (setsockopt(socket, SOL_SOCKET, opt, &tv, sizeof(tv)) < 0) {
    KLOGW(TAG, "set_rw_timeout failed: %s", strerror(errno));
  }
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

#ifdef LIZARD_DEBUG
void print_hex_data(const uint8_t *data, uint32_t size) {
  uint32_t i;
  for (i = 0; i < size; ++i) {
    printf("%x ", data[i]);
  }
  printf("\n");
}
#endif

} // namespace lizard
} // namespace rokid
