#pragma once

#include "node.h"

namespace rokid {
namespace lizard {

class SocketNode : public Node {
public:
  ~SocketNode();

  const char* name() const { return "socket"; }

protected:
  bool on_init(const rokid::Uri& uri, void* arg);

  int32_t on_write(Buffer *in, Buffer *out, void* arg);

  int32_t on_read(Buffer *out, Buffer *in, void *arg);

  void on_close();

private:
  void set_node_error_by_errno();

  void set_node_error(int32_t code);

public:
  static const int32_t ERROR_CODE_BEGIN = -10000;
  static const int32_t NOT_READY = -10000;
  static const int32_t REMOTE_CLOSED = -10001;
  static const int32_t INSUFF_BUFFER = -10002;
  static const int32_t READ_TIMEOUT = -10003;

private:
  int socket = -1;
  static const char* error_messages[4];
};

} // namespace lizard
} // namespace rokid
