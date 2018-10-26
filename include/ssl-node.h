#pragma once

#include "node.h"
#include "ssl.h"
#include "entropy.h"
#include "ctr_drbg.h"

namespace rokid {
namespace lizard {

class SSLNode : public Node {
public:
  ~SSLNode();

  const char* name() const { return "mbedtls"; }

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
  entropy_context entropy;
  ctr_drbg_context ctr_drbg;
  ssl_context ssl;
  int socket = -1;
};

} // namespace lizard
} // namespace rokid
