// 
//
// Much of the following was modeled after Vinnie's Boost Beast examples:


#include "ws_ssl_client.h"
#include "plog/Log.h"

namespace asio_ws {
namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace http = beast::http;           // from <boost/beast/http.hpp>
namespace websocket = beast::websocket; // from <boost/beast/websocket.hpp>
namespace net = boost::asio;            // from <boost/asio.hpp>
namespace ssl = boost::asio::ssl;       // from <boost/asio/ssl.hpp>
using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>

WS_SSL_Client::WS_SSL_Client( net::io_context& ioc,
                       OnMTPReady ready,
                       ErrorCBFunc failed, MsgHandler msgHandler,
                       const WSOptions& options ): ws_client_io{},
                       ioc_{ioc},
                       ctx_{ boost::asio::ssl::context::method::tlsv12_client},
                       ready_{ready}, failed_{failed},
                       msgHandler_{msgHandler}, resolver_{ioc},
                       ws_{ioc, ctx_}, options_{options}
{
}

WS_SSL_Client::~WS_SSL_Client(){}

void WS_SSL_Client::SetTLS( int vmode,
             const std::string* ca_file, const std::string* cert_file,
             const std::string* key_file) {
  tlsMode_ = vmode;
  caFile_ = ca_file;
  certFile_ = cert_file;
  keyFile_ = key_file;
  ctx_.load_verify_file( *ca_file );
  //ctx_.set_verify_mode(boost::asio::ssl::verify_peer);
}

// TODO? timer?
void WS_SSL_Client::Start( const std::string* host, const std::string* port, const std::string* path){
  
  host_ = *host;
  path_ = *path;
  
  resolver_.async_resolve( host_, *port,
   beast::bind_front_handler( &WS_SSL_Client::OnResolve, this)); 
}

void WS_SSL_Client::OnResolve(const beast::error_code& ec, tcp::resolver::results_type results) {
  if (ec) {
    PLOG(plog::error) << "Host resolve failed: " << ec.message();
    return;
  }
  // Set the timeout for the operation
  beast::get_lowest_layer(ws_).expires_after(std::chrono::seconds(30));

  // Make the connection on the IP address we get from a lookup
  beast::get_lowest_layer(ws_).async_connect(
      results,
      beast::bind_front_handler(
          &WS_SSL_Client::OnConnect,
          this));
}

void WS_SSL_Client::OnConnect(const beast::error_code& ec, tcp::resolver::results_type::endpoint_type ep) {
  PLOG(plog::debug);
  if (ec) {
    PLOG(plog::error) << "Connect failed: " << ec.message();
    return;
  }
  // Set a timeout on the operation
  beast::get_lowest_layer(ws_).expires_after(std::chrono::seconds(30));
  // Set SNI Hostname (many hosts need this to handshake successfully -- RFC 3546)
  if (!SSL_set_tlsext_host_name(
          ws_.next_layer().native_handle(), host_.c_str())) {
    beast::error_code ec = beast::error_code(static_cast<int>(::ERR_get_error()),
                           net::error::get_ssl_category());
    failed_(ec, "Write error.");
    return;
  }
  // Update the host_ string. This will provide the value of the
  // Host HTTP header during the WebSocket handshake.
  // See https://tools.ietf.org/html/rfc7230#section-5.4
  host_ += ':' + std::to_string(ep.port());
  // Perform the SSL handshake
  ws_.next_layer().async_handshake(
      ssl::stream_base::client,
      beast::bind_front_handler(
          &WS_SSL_Client::OnSSLHandshake,
          this));
}

void WS_SSL_Client::OnSSLHandshake(const beast::error_code& ec) {
  // Turn off the timeout on the tcp_stream, because
  // the websocket stream has its own timeout system.
  beast::get_lowest_layer(ws_).expires_never();

  // Set suggested timeout settings for the websocket
  ws_.set_option(
      websocket::stream_base::timeout::suggested(
          beast::role_type::client));
  ws_.set_option(websocket::stream_base::decorator(
      [this](websocket::request_type& req) {
        req.set(http::field::user_agent,
                std::string(BOOST_BEAST_VERSION_STRING) +
                    " websocket-client-async");
        // set any additional options provided by client user.
        for( auto p: this->options_) {
          req.set(p.first, *p.second);
        }
      }));

  // Perform the websocket handshake
  ws_.async_handshake(res_, host_, path_,
                      beast::bind_front_handler(
                          &WS_SSL_Client::OnWebSocketHandshake,
                          this) );

}

void WS_SSL_Client::OnWebSocketHandshake(const beast::error_code &ec){
  if ( ec ){
    failed_(ec, "WebSocket Handshake");
    return;
  }
  std::cout << "---- response_type:\n" << res_ << '\n';
  ready_(res_);
}

void WS_SSL_Client::StartRead(){
  ws_.async_read( buffer_, beast::bind_front_handler(
                &WS_SSL_Client::OnRead,
                this)); 
}

void WS_SSL_Client::Write(const unsigned char* msg, const size_t lth){
  ws_.async_write( net::buffer(msg, lth),
        beast::bind_front_handler( &WS_SSL_Client::OnWrite, this));
}

void WS_SSL_Client::OnWrite(const beast::error_code &ec, std::size_t xfered){
  if ( ec ){
    failed_(ec, "Write error.");
    return;
  }

  buffer_.consume(buffer_.size());
  ws_.async_read( buffer_, beast::bind_front_handler(
                &WS_SSL_Client::OnRead,
                this));
  
}

void WS_SSL_Client::OnRead(const beast::error_code &ec, std::size_t bytesXfered ){
  if ( ec ) {
    PLOG(plog::error) << "Read failed: " << ec.message();
    failed_(ec, "Read Failed.");
    return;
  }
  msgHandler_( static_cast<char const*>(buffer_.data().data()), bytesXfered);
  
}

bool WS_SSL_Client::VerifyCertificate(bool preverified,
      boost::asio::ssl::verify_context& ctx)
  {
    // The verify callback can be used to check whether the certificate that is
    // being presented is valid for the peer. For example, RFC 2818 describes
    // the steps involved in doing this for HTTPS. Consult the OpenSSL
    // documentation for more details. Note that the callback is called once
    // for each certificate in the certificate chain, starting from the root
    // certificate authority.

    // In this example we will simply print the certificate's subject name.
    char subject_name[256];
    X509* cert = X509_STORE_CTX_get_current_cert(ctx.native_handle());
    X509_NAME_oneline(X509_get_subject_name(cert), subject_name, 256);
    std::cout << "Verifying " << subject_name << "\n";

    return preverified;
  }


} // namespace asio_ws
