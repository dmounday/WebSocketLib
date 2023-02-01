#ifndef WS_CLIENT_H
#define WS_CLIENT_H

#include "templates.h"
#include "ws_tcp_client.h"
#include "ws_ssl_client.h"

namespace asio_ws {
namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace http = beast::http;           // from <boost/beast/http.hpp>
namespace websocket = beast::websocket; // from <boost/beast/websocket.hpp>
namespace net = boost::asio;            // from <boost/asio.hpp>
using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp
class WS_Client
{
public:
    WS_Client(net::io_context& ioc, OnMTPReady ready,
                       ErrorCBFunc failed, MsgHandler msgHandler,
                       const WSOptions& options );
    ~WS_Client();

/// @brief Set TLS verify mode and certificate file paths. This function should be called
  /// prior to calling Start().
  /// @param  use either SSL_VERIFY_NONE or SSL_VERIFY_PEER, the last 3 options are
  /// 'ored' with SSL_VERIFY_PEER if they are desired
  ///   SSL_VERIFY_NONE                 0x00
  ///   SSL_VERIFY_PEER                 0x01
  ///   SSL_VERIFY_FAIL_IF_NO_PEER_CERT 0x02
  ///   SSL_VERIFY_CLIENT_ONCE          0x04
  ///   SSL_VERIFY_POST_HANDSHAKE       0x08  
  /// @param ca_file: server certificate file path.
  /// @param cert_file: client certificate file path. 
  /// @param key_file : client private key file path.
void SetTLS( int vmode,
             const std::string* ca_file, const std::string* cert_file = nullptr ,
             const std::string* key_file = nullptr);

void Start( const std::string* host, const std::string* port,
           const std::string* path);

void Write(const unsigned char* msg, const size_t lth);

private:
  std::unique_ptr<ws_client_io> ioClient_;
  net::io_context& ioc_;
  OnMTPReady ready_;
  ErrorCBFunc failed_;
  MsgHandler msgHandler_;
  tcp::resolver resolver_;
  websocket::stream<beast::tcp_stream> ws_;
  const WSOptions& options_;
  beast::flat_buffer buffer_;
  std::string host_;
  std::string path_;
  websocket::response_type res_;

  int tlsMode_;
  const std::string* caFile_;
  const std::string* certFile_;
  const std::string* keyFile_;

};

} // namespace
#endif