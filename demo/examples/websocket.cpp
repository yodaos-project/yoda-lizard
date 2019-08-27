#include "sock-node.h"
#include "ssl-node.h"
#include "ws-node.h"
#include "ws-frame.h"

// #define SERVER_URI "ws://localhost:3000/"
// #define SERVER_URI "ws://echo.websocket.org:80/"
#define SERVER_URI "wss://echo.websocket.org:443/"

using namespace rokid;
using namespace rokid::lizard;

int main(int argc, char** argv) {
  SocketNode sock_node;
#ifdef HAS_SSL
  SSLNode ssl_node;
#endif
  WSNode cli;
  Uri uri;
  const NodeError *err;
  char data[128];
  Buffer rbuf(data, 48), wbuf(data + 48, 48), buf(data + 96, 32);

  char mask[4] = { 'a', 'b', 'c', 'd' };
  cli.set_masking_key(mask);
  if (!uri.parse(SERVER_URI)) {
    printf("parse server uri failed\n");
    return 1;
  }
  if (uri.scheme == "wss") {
#ifdef HAS_SSL
    cli.chain(&ssl_node);
#else
    printf("not support ssl\n");
    return 1;
#endif
  } else {
    cli.chain(&sock_node);
  }
  NodeArgs<Buffer> bufs;
  bufs.add(&rbuf);
  cli.set_read_buffers(&bufs);
  bufs.clear();
  bufs.add(&wbuf);
  cli.set_write_buffers(&bufs);
  if (!cli.init(uri)) {
    err = cli.get_error();
    printf("node %s init failed: %s\n", err->node->name(), err->desc.c_str());
    return 1;
  }

  uint32_t wsflags;
  NodeArgs<void> args;
  // echo "hello"
  if (!cli.send_frame("hello", 5)) {
    cli.close();
    err = cli.get_error();
    printf("node %s write failed: %s\n", err->node->name(), err->desc.c_str());
    return 1;
  }
  if (!cli.read(&buf)) {
    cli.close();
    err = cli.get_error();
    printf("node %s read failed: %s\n", err->node->name(), err->desc.c_str());
    return 1;
  }
  reinterpret_cast<char *>(buf.data_end())[0] = '\0';
  printf("node read string %s\n", buf.data_begin());
  buf.clear();

  // echo "world"
  if (!cli.send_frame("world", 5)) {
    cli.close();
    err = cli.get_error();
    printf("node %s write failed: %s\n", err->node->name(), err->desc.c_str());
    return 1;
  }
  if (!cli.read(&buf)) {
    cli.close();
    err = cli.get_error();
    printf("node %s read failed: %s\n", err->node->name(), err->desc.c_str());
    return 1;
  }
  reinterpret_cast<char *>(buf.data_end())[0] = '\0';
  printf("node read string %s\n", buf.data_begin());
  buf.clear();

  if (!cli.ping()) {
    cli.close();
    printf("ping failed\n");
    return 1;
  }
  args.add(&wsflags);
  if (!cli.read(&buf, &args)) {
    cli.close();
    printf("read pong failed\n");
    return 1;
  }
  if ((wsflags & OPCODE_MASK) != OPCODE_PONG
      || !(wsflags & WSFRAME_FIN)) {
    printf("read pong failed: ws flags is %u\n", wsflags);
    cli.close();
    return 1;
  }

  // test timeout
  uint32_t timeout = 1;
  args.add(nullptr);
  args.add(&timeout);
  if (!cli.read(&buf, &args)) {
    err = cli.get_error();
    printf("%s\n", err->desc.c_str());
  }

  cli.close();
  return 0;
}
