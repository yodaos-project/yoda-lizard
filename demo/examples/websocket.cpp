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
  SSLNode ssl_node;
  WSNode cli;
  Uri uri;
  NodeError err;
  Buffer buf;

  char mask[4] = { 'a', 'b', 'c', 'd' };
  cli.set_masking_key(mask);
  if (!uri.parse(SERVER_URI)) {
    printf("parse server uri failed\n");
    return 1;
  }
  if (uri.scheme == "wss")
    cli.chain(&ssl_node);
  else
    cli.chain(&sock_node);
  if (!cli.init(uri, &err)) {
    printf("node %s init failed: %s\n", err.node->name(), err.descript);
    return 1;
  }

  // echo "hello"
  char data[32];
  buf.set_data((char*)"hello", 5, 0, 5);
  if (!cli.write(buf, &err)) {
    cli.close();
    printf("node %s write failed: %s\n", err.node->name(), err.descript);
    return 1;
  }
  buf.set_data(data, sizeof(data), 0, 0);
  if (!cli.read(buf, &err)) {
    cli.close();
    printf("node %s read failed: %s\n", err.node->name(), err.descript);
    return 1;
  }
  data[buf.size()] = '\0';
  printf("node read string %s\n", data);

  // echo "world"
  buf.set_data((char*)"world", 5, 0, 5);
  if (!cli.write(buf, &err)) {
    cli.close();
    printf("node %s write failed: %s\n", err.node->name(), err.descript);
    return 1;
  }
  buf.set_data(data, sizeof(data), 0, 0);
  if (!cli.read(buf, &err)) {
    cli.close();
    printf("node %s read failed: %s\n", err.node->name(), err.descript);
    return 1;
  }
  data[buf.size()] = '\0';
  printf("node read string %s\n", data);

  if (!cli.ping()) {
    cli.close();
    printf("ping failed\n");
    return 1;
  }
  buf.clear();
  uintptr_t wsflags;
  if (!cli.read(buf, &err, 1, (void**)&wsflags)) {
    cli.close();
    printf("read pong failed\n");
    return 1;
  }
  if ((wsflags & OPCODE_MASK) != OPCODE_PONG
      || !(wsflags & WSFRAME_FIN)) {
    printf("read pong failed: ws flags is %lu\n", wsflags);
    cli.close();
    return 1;
  }

  // test timeout
  void* args[2] = { nullptr, (void*)1 };
  buf.clear();
  if (!cli.read(buf, &err, 2, args)) {
    if (err.node == &ssl_node && err.code == SSLNode::SSL_READ_TIMEOUT) {
      printf("ssl read timeout\n");
    } else {
      printf("%s\n", err.descript);
    }
  }

  cli.close();
  return 0;
}
