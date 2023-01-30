

#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/asio/strand.hpp>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <memory>
#include <string>

namespace asio_ws {
namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace http = beast::http;           // from <boost/beast/http.hpp>
namespace websocket = beast::websocket; // from <boost/beast/websocket.hpp>
namespace net = boost::asio;            // from <boost/asio.hpp>
using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>

using OnMTPReady = std::function<void(const websocket::response_type& )>;
using ErrorCBFunc = std::function<void(const boost::system::error_code&, const std::string&)>;
using MsgHandler = std::function<void(const char* buf, const size_t lth)>;
using WSOptions = std::vector<std::pair<http::field, const std::string*>>;

class WS_Client : public std::enable_shared_from_this<WS_Client> {
public:
  /// @brief Web socket client constructor for SSL or plain connect with
  /// a WebSocket server.
  /// @param ioc refence to asio io_context.
  /// @param Callback when client is connected successfully and ready.
  /// @param Callback if client fails to connect.

  WS_Client( net::io_context& ioc, OnMTPReady, ErrorCBFunc, MsgHandler,
              const WSOptions& );

  
  virtual ~WS_Client();

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
             const std::string* ca_file, const std::string* cert_file = nullptr,
             const std::string* key_file = nullptr);

  /// @brief Start the WebServer client code by resolveing host and establish the tcp
  /// connection. Calls the user's ready function when connection is upgraded. 
  /// @param host 
  /// @param port 
  /// @param path Path of WebService. (target of GET with upgrade) 
  void Start( const std::string* host, const std::string* port, const std::string* path);

  /// @brief Write message and wait for read response to complete.
  /// @param msg 
  /// @param lth 
  void Write(const unsigned char* msg, const size_t lth);

  /// @brief Start an async read. Read completes with call to MsgHandler.
  void StartRead();

 private:
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
  const std::string* caFile_;
  const std::string* certFile_;
  const std::string* keyFile_;
  


  void OnResolve(const beast::error_code &ec, tcp::resolver::results_type results);
  void OnConnect(const beast::error_code &ec, tcp::resolver::results_type::endpoint_type ep);
  void OnHandshake(const beast::error_code &ec);

  void OnWrite(const beast::error_code &ec, std::size_t bytesXfered );
  void OnRead(const beast::error_code &ec, std::size_t bytesXfered );


};

} // namespace asio_ws
