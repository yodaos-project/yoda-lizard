#pragma once

#include "node.h"

namespace rokid {
namespace lizard {

class SocketNode : public Node {
public:
  const char* name() const { return "socket"; }

protected:
  bool on_init(rokid::Uri& uri, NodeError* err, void* arg);

  int32_t on_write(Buffer& in, Buffer& out, NodeError* err, void* arg);

  int32_t on_read(Buffer& out, NodeError* err, void** out_arg);

  void on_close();

private:
  void set_node_error(NodeError* err, int32_t code);

  void set_read_timeout(uint32_t tm);

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
