#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include "sock-node.h"

namespace rokid {
namespace lizard {

extern void ignore_sigpipe(int socket);

const char* SocketNode::error_messages[] = {
  "socket not ready",
  "remote socket closed",
  "insufficient buffer capacity",
  "socket read timeout",
};

static void set_node_error_by_errno(NodeError* err) {
  if (err) {
    err->code = errno;
    err->descript = strerror(errno);
  }
}

static void set_read_timeout(int socket, uint32_t tm) {
  struct timeval tv;
  tv.tv_sec = tm / 1000;
  tv.tv_usec = (tm % 1000) * 1000;
  setsockopt(socket, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
}

SocketNode::~SocketNode() {
  on_close();
}

bool SocketNode::on_init(rokid::Uri& uri, NodeError* err, void* arg) {
  int fd = ::socket(AF_INET, SOCK_STREAM, 0);
  if (fd < 0) {
    set_node_error_by_errno(err);
    return false;
  }
  struct sockaddr_in addr;
  struct hostent* hp;
  hp = gethostbyname(uri.host.c_str());
  if (hp == nullptr) {
    set_node_error_by_errno(err);
    ::close(fd);
    return false;
  }
  memset(&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;
  memcpy(&addr.sin_addr, hp->h_addr_list[0], sizeof(addr.sin_addr));
  addr.sin_port = htons(uri.port);
  if (::connect(fd, (sockaddr*)&addr, sizeof(addr)) < 0) {
    set_node_error_by_errno(err);
    ::close(fd);
    return false;
  }
  ignore_sigpipe(fd);
  socket = fd;
  return true;
}

void SocketNode::set_node_error(NodeError* err, int32_t code) {
  if (err) {
    err->code = code;
    err->descript = error_messages[ERROR_CODE_BEGIN - code];
  }
}

int32_t SocketNode::on_write(Buffer& in, Buffer& out, NodeError* err,
    void* arg) {
  if (socket < 0) {
    set_node_error(err, NOT_READY);
    return -1;
  }
  ssize_t r = ::write(socket, in.data_begin(), in.size());
  if (r < 0) {
    set_node_error_by_errno(err);
    return -1;
  }
  if (r == 0) {
    set_node_error(err, REMOTE_CLOSED);
    return -1;
  }
  in.clear();
  return 0;
}

int32_t SocketNode::on_read(Buffer& out, NodeError* err, void** out_arg) {
  if (socket < 0) {
    set_node_error(err, NOT_READY);
    return -1;
  }
  if (out.remain_space() == 0) {
    set_node_error(err, INSUFF_BUFFER);
    return -1;
  }
  if (out_arg) {
    set_read_timeout(socket, (uint32_t)(uintptr_t)(*out_arg));
  }
  ssize_t r = ::read(socket, out.data_end(), out.remain_space());
  if (r < 0) {
    if (errno == EAGAIN) {
      set_node_error(err, READ_TIMEOUT);
    } else {
      set_node_error_by_errno(err);
    }
    return -1;
  }
  if (r == 0) {
    set_node_error(err, REMOTE_CLOSED);
    return -1;
  }
  out.obtain(r);
  return 0;
}

void SocketNode::on_close() {
  if (socket >= 0) {
    ::close(socket);
    socket = -1;
  }
}

} // namespace lizard
} // namespace rokid
