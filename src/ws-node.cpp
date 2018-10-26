#include <strings.h>
#include "ws-node.h"
#include "ws-frame.h"
#include "http.h"

using namespace std;

namespace rokid {
namespace lizard {

const char* WSNode::error_messages[] = {
  "websocket handshark failed",
  "received invalid opcode",
  "control frame with payload data size larger than 125",
  "insufficient websocket frame read buffer",
};

WSNode::WSNode(uint32_t rdbufsize) : write_buffer(0) {
  read_buffer = new MmapBuffer(rdbufsize);
}

WSNode::~WSNode() {
  delete read_buffer;
}

bool WSNode::send_frame(void* payload, uint32_t size, uint32_t flags) {
  Buffer in;
  uintptr_t arg = flags;
  in.set_data(payload, size, 0, size);
  return Node::write(in, nullptr, 1, (void**)&arg);
}

bool WSNode::ping(void* payload, uint32_t size) {
  return send_frame(payload, size, OPCODE_PING | WSFRAME_FIN);
}

bool WSNode::pong(void* payload, uint32_t size) {
  return send_frame(payload, size, OPCODE_PONG | WSFRAME_FIN);
}

void WSNode::set_masking_key(const char* key) {
  memcpy(masking_key, key, 4);
}

void WSNode::set_node_error(NodeError* err, int32_t code) {
  if (err) {
    err->code = code;
    err->descript = error_messages[ERROR_CODE_BEGIN - code];
  }
}

bool WSNode::on_init(rokid::Uri& uri, NodeError* err, void* arg) {
  HttpRequest req;
  char buf[256];
  int32_t len;

  req.setPath(uri.path.c_str());
  snprintf(buf, sizeof(buf), "%s:%d", uri.host.c_str(), uri.port);
  req.addHeaderField("Host", buf);
  req.addHeaderField("Upgrade", "websocket");
  req.addHeaderField("Connection", "Upgrade");
  req.addHeaderField("Sec-WebSocket-Key", "x3JJHMbDL1EzLkh9GBhXDw==");
  req.addHeaderField("Sec-WebSocket-Version", "13");
  len = req.build(buf, sizeof(buf));
  if (len <= 0) {
    goto failed;
  }
  if (super_node) {
    HttpResponse resp;
    map<string, string>::iterator it;
    Buffer rwbuf;

    rwbuf.set_data(buf, sizeof(buf), 0, len);
    if (!super_node->write(rwbuf, err)) {
      return false;
    }
    int32_t pr;
    while (true) {
      if (!super_node->read(rwbuf, err)) {
        return false;
      }
      pr = resp.parse((char*)rwbuf.data_begin(), rwbuf.size());
      if (pr == HttpResponse::NOT_FINISH) {
        continue;
      }
      if (pr > 0) {
        break;
      }
      goto failed;
    }
    if (strcmp(resp.statusCode, "101"))
      goto failed;
    it = resp.headerFields.find("Upgrade");
    if (it == resp.headerFields.end())
      goto failed;
    if (strcasecmp(it->second.c_str(), "websocket"))
      goto failed;
    it = resp.headerFields.find("Connection");
    if (it == resp.headerFields.end())
      goto failed;
    if (strcasecmp(it->second.c_str(), "upgrade"))
      goto failed;
    // TODO: check field Sec-WebSocket-Accept
  }
  return true;

failed:
  set_node_error(err, HANDSHARK_FAILED);
  return false;
}

int32_t WSNode::on_write(Buffer& in, Buffer& out, NodeError* err,
    void* arg) {
  uint32_t flags = (uintptr_t)arg;
  if (write_state == 0) {
    uint8_t mask = *(int32_t*)masking_key ? 1 : 0;
    int32_t c = lizard_ws_frame_create(flags & OPCODE_MASK,
        flags & FIN_MASK ? 1 : 0, mask, masking_key,
        in.size(), frame_header, sizeof(frame_header));
    out.set_data(frame_header, sizeof(frame_header), 0, c);
    write_state = 1;
    return 1;
  }
  if (*(int32_t*)masking_key) {
    uint32_t wsize;
    if (is_control_opcode(flags & OPCODE_MASK) && in.size() > 125) {
      set_node_error(err, INVALID_CONTROL_FRAME_FORMAT);
      return -1;
    }
    if (in.size() > write_buffer.total_space()) {
      wsize = write_buffer.total_space();
    } else {
      wsize = in.size();
    }
    lizard_ws_frame_mask_payload(masking_key, in.data_begin(),
        wsize, write_buffer.data_begin());
    in.consume(wsize);
    write_buffer.obtain(wsize);
    out.move(write_buffer);
    if (!in.empty()) {
      return 1;
    }
    write_state = 0;
    return 0;
  }
  out.move((Buffer&)in);
  write_state = 0;
  return 0;
}

int32_t WSNode::on_read(Buffer& out, NodeError* err, void** out_arg) {
  uint32_t read_bytes = read_buffer->size();
  uint8_t* p = (uint8_t*)read_buffer->data_begin();
  WSFrameHeader header;
  int32_t hsz = lizard_ws_frame_parse_header(p, read_bytes, &header);
  if (hsz == -1) {
    set_node_error(err, INVALID_OPCODE);
    return hsz;
  } else if (hsz == -2) {
    set_node_error(err, INVALID_CONTROL_FRAME_FORMAT);
    return hsz;
  } else if (hsz == 0) {
    return 1;
  } else if (hsz < 0) {
    return hsz;
  }
  uint64_t frame_size = lizard_ws_frame_size(&header);
  if (frame_size > read_bytes)
    return 1;
  out.shift();
  if (out.remain_space() < header.payload_length) {
    set_node_error(err, INSUFF_READ_BUFFER);
    return -1;
  }
  if (header.mask) {
    lizard_ws_frame_mask_payload((char*)(p + hsz), p + hsz + 4, header.payload_length,
        out.data_begin());
    out.obtain(header.payload_length);
    read_buffer->consume(hsz + 4 + header.payload_length);
  } else {
    out.append(p + hsz, header.payload_length);
    read_buffer->consume(hsz + header.payload_length);
  }
  if (out_arg) {
    intptr_t v = header.opcode;
    if (header.fin)
      v |= WSFRAME_FIN;
    *out_arg = (void*)v;
  }
  return 0;
}

void WSNode::on_close() {
  read_buffer->clear();
}

} // namespace lizard
} // namespace rokid
