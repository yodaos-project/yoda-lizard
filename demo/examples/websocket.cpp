#include "sock-node.h"
#include "ssl-node.h"
#include "ws-node.h"

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
  buf.set_data((char*)"hello", 5, 0, 5);
  if (!cli.write(buf, &err)) {
    cli.close();
    printf("node %s write failed: %s\n", err.node->name(), err.descript);
    return 1;
  }
  char data[32];
  buf.set_data(data, sizeof(data), 0, 0);
  if (!cli.read(buf, &err)) {
    cli.close();
    printf("node %s read failed: %s\n", err.node->name(), err.descript);
    return 1;
  }
  data[buf.end] = '\0';
  printf("node read string %s\n", data);
  cli.close();
  return 0;
}
