#pragma once

#include "node.h"

namespace rokid {
namespace lizard {

class WSNode : public Node {
public:
  WSNode(uint32_t rdbufsize = 0);

  ~WSNode();

  // 0x12 == OPCODE_BINARY | WSFRAME_FIN
  bool send_frame(void* payload, uint32_t size, uint32_t flags = 0x12);

  bool ping(void* payload = nullptr, uint32_t size = 0);

  bool pong(void* payload = nullptr, uint32_t size = 0);

  void set_masking_key(const char* key);

  const char* name() const { return "websocket"; }

protected:
  bool on_init(rokid::Uri& uri, NodeError* err, void* arg);

  int32_t on_write(Buffer& in, Buffer& out, NodeError* err, void* arg);

  int32_t on_read(Buffer& out, NodeError* err, void** out_arg);

  void on_close();

private:
  void set_node_error(NodeError* err, int32_t code);

public:
  static const int32_t ERROR_CODE_BEGIN = -10000;
  static const int32_t HANDSHARK_FAILED = -10000;
  static const int32_t INVALID_OPCODE = -10001;
  static const int32_t INVALID_CONTROL_FRAME_FORMAT = -10002;
  static const int32_t INSUFF_READ_BUFFER = -10003;

private:
  static const char* error_messages[4];

  MmapBuffer write_buffer;
  uint32_t read_frame_header_size = 0;
  uint32_t excepted_read_payload_data_size = 0;
  // 0: write websocket frame header
  // 1: write websocket frame payload data
  int32_t write_state = 0;
  char masking_key[4] = {0};
  char frame_header[14];
};

} // namespace lizard
} // namespace rokid
