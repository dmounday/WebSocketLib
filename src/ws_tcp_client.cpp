// 
//
// Much of the following was modeled after Vinnie's Boost Beast examples:


#include "ws_client.h"
#include "plog/Log.h"

namespace asio_ws {
//void SetOptions(websocket::request_type& req){
//}

WS_TCP_Client::WS_TCP_Client( net::io_context& ioc, OnMTPReady ready,
                       ErrorCBFunc failed, MsgHandler msgHandler,
                       const WSOptions& options ): ws_client_io{},
                       ioc_{ioc}, ready_{ready}, failed_{failed},
                       msgHandler_{msgHandler}, resolver_{ioc},
                       ws_{ioc}, options_{options}
{
}

WS_TCP_Client::~WS_TCP_Client(){}


// TODO? timer?
void WS_TCP_Client::Start( const std::string* host, const std::string* port, const std::string* path){
  host_ = *host;
  path_ = *path;
  resolver_.async_resolve( host_, *port,
   beast::bind_front_handler( &WS_TCP_Client::OnResolve, this)); 
}

void WS_TCP_Client::StartRead(){
  ws_.async_read( buffer_, beast::bind_front_handler(
                &WS_TCP_Client::OnRead,
                this)); 
}

void WS_TCP_Client::OnResolve(const beast::error_code& ec, tcp::resolver::results_type results) {
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
          &WS_TCP_Client::OnConnect,
          this));
}

void WS_TCP_Client::OnConnect(const beast::error_code& ec, tcp::resolver::results_type::endpoint_type ep) {
  PLOG(plog::debug);
  if (ec) {
    PLOG(plog::error) << "Connect failed: " << ec.message();
    return;
  }

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

  // Update the host_ string. This will provide the value of the
  // Host HTTP header during the WebSocket handshake.
  // See https://tools.ietf.org/html/rfc7230#section-5.4
  host_ += ':' + std::to_string(ep.port());
  // Perform the websocket handshake
  ws_.async_handshake(res_, host_, path_,
                      beast::bind_front_handler(
                          &WS_TCP_Client::OnWebSocketHandshake,
                          this) );

}

void WS_TCP_Client::OnWebSocketHandshake(const beast::error_code &ec){
  if ( ec ){
    failed_(ec, "handshake");
    return;
  }
  std::cout << "---- response_type:\n" << res_ << '\n';
  ready_(res_);
}

void WS_TCP_Client::Write(const unsigned char* msg, const size_t lth){
  ws_.async_write( net::buffer(msg, lth),
        beast::bind_front_handler( &WS_TCP_Client::OnWrite, this));
}

void WS_TCP_Client::OnWrite(const beast::error_code &ec, std::size_t xfered){
  if ( ec ){
    failed_(ec, "Write error.");
    return;
  }

  buffer_.consume(buffer_.size());
  ws_.async_read( buffer_, beast::bind_front_handler(
                &WS_TCP_Client::OnRead,
                this));
  
}

void WS_TCP_Client::OnRead(const beast::error_code &ec, std::size_t bytesXfered ){
  if ( ec ) {
    PLOG(plog::error) << "Read failed: " << ec.message();
    failed_(ec, "Read Failed.");
    return;
  }
  msgHandler_( static_cast<char const*>(buffer_.data().data()), bytesXfered);
  
}

} // namespace asio_ws
