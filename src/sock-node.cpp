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

void SocketNode::set_read_timeout(uint32_t tm) {
  read_timeout = tm;
  if (socket >= 0) {
    struct timeval tv;
    tv.tv_sec = tm / 1000;
    tv.tv_usec = (tm % 1000) * 1000;
    setsockopt(socket, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
  }
}

bool SocketNode::on_init(rokid::Uri& uri, NodeError* err) {
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
  socket = fd;
  set_read_timeout(read_timeout);
  return true;
}

void SocketNode::set_node_error(NodeError* err, int32_t code) {
  if (err) {
    err->code = code;
    err->descript = error_messages[ERROR_CODE_BEGIN - code];
  }
}

int32_t SocketNode::on_write(Buffer& in, Buffer& out, NodeError* err) {
  if (socket < 0) {
    set_node_error(err, NOT_READY);
    return -1;
  }
  ssize_t r = ::write(socket, in.data + in.begin, in.end - in.begin);
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

int32_t SocketNode::on_read(Buffer& out, NodeError* err,
    void* super_extra, void** extra) {
  if (socket < 0) {
    set_node_error(err, NOT_READY);
    return -1;
  }
  if (out.capacity == out.end) {
    set_node_error(err, INSUFF_BUFFER);
    return -1;
  }
  ssize_t r = ::read(socket, out.data + out.end, out.capacity - out.end);
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
  out.end += r;
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
