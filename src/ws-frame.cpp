#include <string.h>
#include <arpa/inet.h>
#include "ws-frame.h"

#ifdef __APPLE__
#include <libkern/OSByteOrder.h>
#define htobe64(x) OSSwapHostToBigInt64(x)
#define be64toh(x) OSSwapBigToHostInt64(x)
#endif

int32_t check_opcode(uint8_t op) {
  return op <= OPCODE_BINARY || (op >= OPCODE_CLOSE && op <= OPCODE_PONG);
}

int32_t is_control_opcode(uint8_t op) {
  return op >= OPCODE_CLOSE && op <= OPCODE_PONG;
}

int32_t lizard_ws_frame_create(uint16_t op, uint8_t fin, uint8_t mask,
    const char* mask_key, uint64_t payload_len, void* out, uint32_t out_size) {
  if (!check_opcode(op))
    return WSFRAME_ERROR_INVALID_OPCODE;
  if (out == nullptr && out_size)
    return WSFRAME_ERROR_INVALID_PARAM;
  if (mask && mask_key == nullptr)
    return WSFRAME_ERROR_INVALID_PARAM;
  bool has_mask = mask && payload_len;
  int16_t header_len;
  int16_t plen_type;
  if (payload_len < 126) {
    header_len = 2;
    plen_type = 0;
  } else if (payload_len <= 0xffff) {
    header_len = 4;
    plen_type = 1;
  } else {
    header_len = 10;
    plen_type = 2;
  }
  if (has_mask)
    header_len += 4;
  if (out_size < header_len)
    return header_len;
  uint8_t* p = reinterpret_cast<uint8_t*>(out);
  p[0] = (fin ? 0x80 : 0) | op;
  switch (plen_type) {
    case 0:
      p[1] = (has_mask ? 0x80 : 0) | (int32_t)payload_len;
      p += 2;
      break;
    case 1: {
      uint16_t s;
      p[1] = (has_mask ? 0x80 : 0) | 126;
      s = htons(payload_len);
      memcpy(p + 2, &s, sizeof(s));
      p += 4;
      break;
    }
    case 2: {
      uint64_t ll;
      p[1] = (has_mask ? 0x80 : 0) | 127;
      ll = htobe64(payload_len);
      memcpy(p + 2, &ll, sizeof(ll));
      p += 10;
      break;
    }
  }
  if (has_mask) {
    memcpy(p, mask_key, 4);
  }
  return header_len;
}

void lizard_ws_frame_mask_payload(const char* mask_key, const void* in, uint32_t in_size, void* out) {
  const char* inp = reinterpret_cast<const char*>(in);
  char* outp = reinterpret_cast<char*>(out);
  uint32_t i;

  for (i = 0; i < in_size; ++i) {
    outp[i] = inp[i] ^ mask_key[i % 4];
  }
}

int32_t lizard_ws_frame_parse_header(uint8_t* data, uint32_t size, WSFrameHeader* result) {
  if (size == 0)
    return 0;
  uint32_t hsz;
  uint8_t opcode = data[0] & 0xf;
  if (!check_opcode(opcode)) {
    return -1;
  }
  result->payload_length = data[1] & 0x7f;
  result->type = 0;
  hsz = 2;
  if (result->payload_length == 126) {
    if (is_control_opcode(opcode))
      return -2;
    if (size < 4)
      return 0;
    uint16_t raw_n;
    memcpy(&raw_n, data + 2, sizeof(raw_n));
    result->payload_length = ntohs(raw_n);
    result->type = 1;
    hsz = 4;
  } else if (result->payload_length == 127) {
    if (is_control_opcode(opcode))
      return -2;
    if (size < 10)
      return 0;
    memcpy(&result->payload_length, data + 2, sizeof(result->payload_length));
    result->payload_length = be64toh(result->payload_length);
    result->type = 2;
    hsz = 10;
  }
  result->fin = (data[0] & 0x80) ? 1 : 0;
  result->mask = (data[1] & 0x80) ? 1 : 0;
  result->opcode = opcode;
  return hsz;
}

uint64_t lizard_ws_frame_size(WSFrameHeader* header) {
  uint64_t frame_size;

  if (header->type == 0) {
    frame_size = 2;
  } else if (header->type == 1) {
    frame_size = 4;
  } else {
    frame_size = 10;
  }
  if (header->mask) {
    frame_size += 4;
  }
  frame_size += header->payload_length;
  return frame_size;
}
