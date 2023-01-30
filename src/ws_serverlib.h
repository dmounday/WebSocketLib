#ifndef WS_SERVERLIB_H_ 
#define WS_SERVERLIB_H_

// 
// This code is mostly a copy of Vinnie Falco's beast webserver modified for
// interfacing to the testing framework. Thus,
// I've included his copyright notice and license reference.
// Thanks Vinnie.
// Copyright (c) 2016-2019 Vinnie Falco (vinnie dot falco at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/boostorg/beast
//
#include <algorithm>
#include <boost/asio/bind_executor.hpp>
#include <boost/asio/dispatch.hpp>
#include <boost/asio/signal_set.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/asio/strand.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/beast/version.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/make_unique.hpp>
#include <boost/optional.hpp>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <vector>

namespace asio_ws {
namespace beast = boost::beast;          // from <boost/beast.hpp>
namespace http = beast::http;            // from <boost/beast/http.hpp>
namespace websocket = beast::websocket;  // from <boost/beast/websocket.hpp>
namespace net = boost::asio;             // from <boost/asio.hpp>
namespace ssl = boost::asio::ssl;        // from <boost/asio/ssl.hpp>
using tcp = boost::asio::ip::tcp;        // from <boost/asio/ip/tcp.hpp>

// Address of send function is passed thru callback to the ReadyCBFunc.
using WebSocketSendFunc = std::function<void(const char*, const size_t)>;

// Server config info injected into the ws_serverlib.
struct Envelope{const std::string* destination; const std::string* message;};

using MsgHandlerFunc = std::function<Envelope(unsigned char*, const size_t)>;
using ReadyCBFunc = std::function<void(WebSocketSendFunc)>;
using ErrorCBFunc = std::function<void(const boost::system::error_code&, const std::string&)>;
using WSOptions = std::vector<std::pair<http::field, const std::string*>>;
using WSOptionsFunc = std::function<WSOptions*()>;
using WSPathFunc = std::function<const std::string*()>;

struct WSDope {
  // Path to start a WebSocket session.
  inline WSDope(WSPathFunc path, MsgHandlerFunc mcbf,
            ReadyCBFunc rcbf, ErrorCBFunc ecbf, WSOptionsFunc opts):
      ws_path_{path}, handler_{mcbf}, ready_{rcbf},
      failed_{ecbf}, ws_options_{opts}{};
  WSPathFunc ws_path_;
  // Callback for messages received by Web Socket session
  MsgHandlerFunc handler_;
  // Callback that indicates WebSocket session is requested.
  ReadyCBFunc ready_;
  // Callback when a failure occures.
  ErrorCBFunc failed_;
  // A vector of WebSocket response options (websocket::response_type)
  WSOptionsFunc ws_options_;
};


// Accepts incoming connections and launches the sessions
class listener : public std::enable_shared_from_this<listener> {
  net::io_context& ioc_;
  ssl::context& ctx_;
  tcp::acceptor acceptor_;
  std::shared_ptr<WSDope> const ws_dope_;

 public:
  listener(
      net::io_context& ioc,
      ssl::context& ctx,
      tcp::endpoint endpoint,
      std::shared_ptr<WSDope> const ws_dope);

  // Start accepting incoming connections
  void run();

 private:
  void
  do_accept();

  void
  on_accept(beast::error_code ec, tcp::socket socket);
};

}  // namespace asio_ws
#endif // WS_SERVERLIB_H_
