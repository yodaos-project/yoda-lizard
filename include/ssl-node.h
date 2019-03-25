#pragma once

#ifdef HAS_SSL

#include <mutex>
#include "node.h"

namespace rokid {
namespace lizard {

class SSLNode : public Node {
public:
  ~SSLNode();

  const char* name() const { return "mbedtls"; }

protected:
  bool on_init(const rokid::Uri& uri, void* arg);

  int32_t on_write(Buffer *in, Buffer *out, void *arg);

  int32_t on_read(Buffer *out, Buffer *in, void *arg);

  void on_close();

private:
  void set_node_error(int32_t code);

public:
  static const int32_t ERROR_CODE_BEGIN = -10000;
  static const int32_t SSL_INIT_FAILED = -10000;
  static const int32_t SSL_HANDSHAKE_FAILED = -10001;
  static const int32_t SSL_WRITE_FAILED = -10002;
  static const int32_t SSL_READ_FAILED = -10003;
  static const int32_t NOT_READY = -10004;
  static const int32_t INSUFF_READ_BUFFER = -10005;
  static const int32_t REMOTE_CLOSED = -10006;
  static const int32_t SSL_READ_TIMEOUT = -10007;

private:
  static const char* error_messages[8];
  void *ssl_data;
  int socket = -1;
};

} // namespace lizard
} // namespace rokid

#endif // HAS_SSL
