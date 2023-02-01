#ifndef WS_CLIENT_IO_H
#define WS_CLIENT_IO_H

#include <boost/beast/core.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/beast/websocket/ssl.hpp>
#include <boost/beast/ssl/ssl_stream.hpp>
#include <boost/asio/ssl/stream.hpp>
#include <boost/asio/ssl/context.hpp>
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

class ws_client_io
{
public:
    ws_client_io();
    virtual ~ws_client_io();

  /// @brief Start the WebServer client code by resolveing host and establish the tcp
  /// connection. Calls the user's ready function when connection is upgraded. 
  /// @param host 
  /// @param port 
  /// @param path Path of WebService. (target of GET with upgrade) 
  virtual void Start( const std::string* host, const std::string* port, const std::string* path) = 0;

  /// @brief Write message and wait for read response to complete.
  /// @param msg 
  /// @param lth 
  virtual void Write(const unsigned char* msg, const size_t lth) = 0;

  /// @brief Start an async read. Read completes with call to MsgHandler.
  virtual void StartRead()=0;
private:

};

} //namespace asio_ws
#endif