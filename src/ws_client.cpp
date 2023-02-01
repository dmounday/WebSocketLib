
#include "ws_client.h"
#include "plog/Log.h"

namespace asio_ws {
namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace http = beast::http;           // from <boost/beast/http.hpp>
namespace websocket = beast::websocket; // from <boost/beast/websocket.hpp>
namespace net = boost::asio;            // from <boost/asio.hpp>
using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp

WS_Client::WS_Client( net::io_context& ioc, OnMTPReady ready,
                       ErrorCBFunc failed, MsgHandler msgHandler,
                       const WSOptions& options ): 
                       ioc_{ioc}, ready_{ready}, failed_{failed},
                       msgHandler_{msgHandler}, resolver_{ioc},
                       ws_{ioc}, options_{options}
{
}

WS_Client::~WS_Client()
{
}

  void WS_Client::SetTLS( int vmode,
             const std::string* ca_file, const std::string* cert_file,
             const std::string* key_file) {
  tlsMode_ = vmode;
  caFile_ = ca_file;
  certFile_ = certFile_;
  keyFile_ = key_file;
}
   

void WS_Client::Start( const std::string* host, const std::string* port,
           const std::string* path){
  if ( !host || !port || !path)
    return;
  if ( caFile_ ) {
    ioClient_ = make_unique<WS_SSL_Client>(ioc_, ready_, failed_, msgHandler_, options_);

  } else {
    ioClient_ = make_unique<WS_TCP_Client>(ioc_, ready_, failed_, msgHandler_, options_);
  }
  if ( ioClient_ ){
    ioClient_->Start(host, port, path);
  }
}

void WS_Client::Write(const unsigned char* msg, const size_t lth){
  ioClient_->Write( msg, lth);
}


} // namespace
// namespace asio_ws