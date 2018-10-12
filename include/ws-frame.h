#pragma once

#define OPCODE_CONT 0
#define OPCODE_TEXT 1
#define OPCODE_BINARY 2
#define OPCODE_CLOSE 8
#define OPCODE_PING 9
#define OPCODE_PONG 10

#define WSFRAME_ERROR_INVALID_OPCODE -1
#define WSFRAME_ERROR_INVALID_PARAM -2

// return: 0  opcode invalid
//         1  opcode valid
int32_t check_opcode(uint8_t op);

int32_t is_control_opcode(uint8_t op);

// return frame length if success
int32_t lizard_ws_frame_create(uint16_t op, uint8_t fin, uint8_t mask,
    const char* mask_key, uint64_t payload_len, void* out, uint32_t out_size);

void lizard_ws_frame_mask_payload(const char* mask_key, const void* in, uint32_t in_size, void* out);

typedef struct {
  uint64_t payload_length;
  uint8_t fin:1;
  uint8_t mask:1;
  uint8_t type:2;
  uint8_t opcode:4;
} WSFrameHeader;
// return: > 0  success, header size
//         0  data not completed
//         -1 data format invalid: opcode invalid
//         -2 data format invalid: control frame, payload length > 125
int32_t lizard_ws_frame_parse_header(uint8_t* data, uint32_t size, WSFrameHeader* result);

uint64_t lizard_ws_frame_size(WSFrameHeader* header);
