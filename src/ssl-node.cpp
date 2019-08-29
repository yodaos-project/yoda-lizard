#ifdef HAS_SSL

#include <errno.h>
#include <string.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include "ssl.h"
#include "entropy.h"
#include "ctr_drbg.h"
#include "ssl-node.h"
#include "common.h"

namespace rokid {
namespace lizard {

extern void ignore_sigpipe(int socket);

const char* SSLNode::error_messages[] = {
  "ssl initialize failed",
  "ssl handshake failed",
  "ssl write failed",
  "ssl read failed",
  "socket not initialized",
  "read buffer size insufficient",
  "remote socket closed",
  "ssl read timeout",
};

SSLNode::~SSLNode() {
  on_close();
}

static int my_net_recv(void* ctx, unsigned char* buf, size_t len) {
  int fd = *(int*)ctx;
  ssize_t ret = ::read(fd, buf, len);
  if (ret < 0) {
    // if socket read timeout, errno will be EAGAIN
    if (errno == EAGAIN)
      return POLARSSL_ERR_NET_WANT_READ;
    return POLARSSL_ERR_NET_RECV_FAILED;
  }
  return ret;
}

class mbedtlsData {
public:
  entropy_context entropy;
  ctr_drbg_context ctr_drbg;
  ssl_context ssl;
  x509_crt cacert;
  bool initialized = false;

  ~mbedtlsData() {
    if (initialized) {
      x509_crt_free(&cacert);
      ssl_free(&ssl);
      ctr_drbg_free(&ctr_drbg);
      entropy_free(&entropy);
    }
  }

  bool init(const std::string& host, const char* ca_list) {
    static const char* pers = "lizard_ssl_node";

    memset(&ssl, 0, sizeof(ssl_context));
    entropy_init(&entropy);
    if(ctr_drbg_init(&ctr_drbg, entropy_func, &entropy,
          (const unsigned char *) pers, strlen(pers)) != 0) {
      entropy_free(&entropy);
      return false;
    }
    if(ssl_init(&ssl) != 0) {
      ctr_drbg_free(&ctr_drbg);
      entropy_free(&entropy);
      return false;
    }
    x509_crt_init(&cacert);
    if (ca_list) {
      if (x509_crt_parse(&cacert, (const unsigned char*)ca_list,
            strlen(ca_list)) < 0) {
        x509_crt_free(&cacert);
        ctr_drbg_free(&ctr_drbg);
        entropy_free(&entropy);
        return false;
      }
      ssl_set_authmode(&ssl, SSL_VERIFY_REQUIRED);
      ssl_set_ca_chain(&ssl, &cacert, nullptr, host.c_str());
    }
    ssl_set_endpoint(&ssl, SSL_IS_CLIENT);
    ssl_set_rng(&ssl, ctr_drbg_random, &ctr_drbg);
    initialized = true;
    return true;
  }
};

bool SSLNode::on_init(const rokid::Uri& uri, void* arg) {
  intptr_t* sslargs = (intptr_t*)arg;
  char* ca_list = (char*)sslargs[0];
  mbedtlsData *mbedtls_data = new mbedtlsData();
  if (!mbedtls_data->init(uri.host, ca_list)) {
    delete mbedtls_data;
    set_node_error(SSL_INIT_FAILED);
    return false;
  }
  KLOGD(TAG, "net_connect %s:%d", uri.host.c_str(), uri.port);
  int r;
  if (r = net_connect(&socket, uri.host.c_str(), uri.port)) {
    KLOGI(TAG, "net_connect failed: -0x%x", -r);
    delete mbedtls_data;
    set_node_error(SSL_INIT_FAILED);
    return false;
  }
  KLOGD(TAG, "set socket write timeout %d", (int32_t)sslargs[1]);

  set_rw_timeout(socket, sslargs[1], true);
  ssl_set_bio(&mbedtls_data->ssl, my_net_recv, &socket, net_send, &socket);
  do {
    r = ssl_handshake(&mbedtls_data->ssl);
    // if (r == POLARSSL_ERR_NET_WANT_WRITE || r == POLARSSL_ERR_NET_WANT_READ)
    //   continue;
    if (r < 0) {
      KLOGI(TAG, "ssl handshake failed: -0x%x", -r);
      set_node_error(SSL_HANDSHAKE_FAILED);
      net_close(socket);
      socket = -1;
      delete mbedtls_data;
      return false;
    }
    break;
  } while (true);
  KLOGD(TAG, "ssl handshake success");
  ignore_sigpipe(socket);
  ssl_data = mbedtls_data;
  return true;
}

void SSLNode::set_node_error(int32_t code) {
  err_info.node = this;
  err_info.code = code;
  err_info.desc = error_messages[ERROR_CODE_BEGIN - code];
}

int32_t SSLNode::on_write(Buffer *in, Buffer *out, void* arg) {
  int r;
  if (in == nullptr || in->empty())
    return 0;
#ifdef LIZARD_DEBUG
  uint32_t sz = in->size();
  uint8_t *db = reinterpret_cast<uint8_t *>(in->data_begin());
#endif
  if (arg) {
    set_rw_timeout(socket, reinterpret_cast<int32_t *>(arg)[0], false);
  } else {
    set_rw_timeout(socket, -1, false);
  }
  while (true) {
    r = ssl_write(&reinterpret_cast<mbedtlsData*>(ssl_data)->ssl, (unsigned char*)in->data_begin(), in->size());
    if (r >= 0) {
      in->consume(r);
      if (in->empty())
        break;
    } else {
      KLOGI(TAG, "ssl write failed: -0x%x", -r);
      set_node_error(SSL_WRITE_FAILED);
      return -1;
    }
  }
#ifdef LIZARD_DEBUG
  // printf("ssl-node: write %u bytes: ", sz);
  // print_hex_data(db, sz);
#endif
  return 0;
}

int32_t SSLNode::on_read(Buffer *out, Buffer *in, void* arg) {
  if (socket < 0) {
    set_node_error(NOT_READY);
    return -1;
  }
  if (out == nullptr || out->remain_space() == 0) {
    set_node_error(INSUFF_READ_BUFFER);
    return -1;
  }
  if (arg) {
    set_rw_timeout(socket, reinterpret_cast<int32_t *>(arg)[0], true);
  } else {
    set_rw_timeout(socket, -1, true);
  }

  int ret;
  do {
    ret = ssl_read(&reinterpret_cast<mbedtlsData*>(ssl_data)->ssl, (unsigned char*)out->data_end(), out->remain_space());

    if (ret == POLARSSL_ERR_NET_WANT_READ) {
      set_node_error(SSL_READ_TIMEOUT);
      return -1;
    }

    if(ret < 0) {
      set_node_error(SSL_READ_FAILED);
      return -1;
    }

    if(ret == 0) {
      set_node_error(REMOTE_CLOSED);
      return -1;
    }

    out->obtain(ret);
#ifdef LIZARD_DEBUG
    // printf("ssl-node: read %d bytes: ", ret);
    // print_hex_data(reinterpret_cast<uint8_t *>(out->data_begin()), out->size());
#endif
    break;
  } while(true);

  return 0;
}

void SSLNode::on_close() {
  if (socket >= 0) {
    net_close(socket);
    delete reinterpret_cast<mbedtlsData*>(ssl_data);
    socket = -1;
  }
}

} // namespace lizard
} // namespace rokid

#endif // HAS_SSL
