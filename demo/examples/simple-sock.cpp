#include "sock-node.h"

#define SERVER_URI "tcp://localhost:30001/"

using namespace rokid;
using namespace rokid::lizard;

int main(int argc, char** argv) {
  SocketNode cli;
  Uri uri;
  Buffer buf;

  if (!uri.parse(SERVER_URI)) {
    printf("parse server uri failed\n");
    return 1;
  }
  if (!cli.init(uri)) {
    printf("node init failed: %s\n", cli.get_error()->desc.c_str());
    return 1;
  }
  buf.set_data((char*)"hello", 5, 0, 5);
  if (!cli.write(&buf)) {
    cli.close();
    printf("node write failed: %s\n", cli.get_error()->desc.c_str());
    return 1;
  }
  char data[32];
  buf.set_data(data, sizeof(data), 0, 0);
  if (!cli.read(&buf)) {
    cli.close();
    printf("node read failed: %s\n", cli.get_error()->desc.c_str());
    return 1;
  }
  data[buf.size()] = '\0';
  printf("node read string %s\n", data);
  cli.close();
  return 0;
}
