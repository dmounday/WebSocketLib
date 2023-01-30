
#include <iomanip>
#include "ws_serverlib.h"
#include "plog/Log.h"
#include "plog/Init.h"
#include "plog/Formatters/TxtFormatter.h"
#include <plog/Appenders/ConsoleAppender.h>
#include "Config.h"


namespace beast = boost::beast;          // from <boost/beast.hpp>
namespace http = beast::http;            // from <boost/beast/http.hpp>
namespace websocket = beast::websocket;  // from <boost/beast/websocket.hpp>
namespace net = boost::asio;             // from <boost/asio.hpp>
namespace ssl = boost::asio::ssl;        // from <boost/asio/ssl.hpp>
using tcp = boost::asio::ip::tcp;        // from <boost/asio/ip/tcp.hpp>
using namespace asio_ws;

const std::string USP_Proto{"v1.usp"};
const std::string USP_Ext{"bbf-usp-protocol; eid="};

class WebSocketServer {
  net::io_context& ioc_;
  WSOptions ws_options_;
  boost::asio::steady_timer timer_;
  std::shared_ptr<listener> server_;
  std::string ws_path_;
  std::string web_socket_ext_; // sec_web_socket_extensions for USP.
  WebSocketSendFunc send_; 

  const std::string* WSPath() {
    PLOG(plog::debug) << ws_path_;
    return &ws_path_;
  }

  WSOptions* GetWSOptions() {
    PLOG(plog::debug);
    return &ws_options_;
  }

  void error_callback(const boost::system::error_code& ec, const std::string& what) {
    PLOG(plog::error) << ec.message() << ": " << what;
    return;
  }

  Envelope msg_handler_func(const unsigned char* msg, size_t lth) {
    PLOG(plog::debug);
    std::cout << msg;
    return Envelope{nullptr, nullptr};
  }

  void ws_connection_ready(WebSocketSendFunc func) {
    PLOG(plog::debug);
    send_ = func;
    /*
    for (auto const& field : req) {
      std::cout << field.name() << " : " << field.value() << '\n';
    }
    std::cout << "Sec-Protocol: " << req[http::field::sec_websocket_protocol] << '\n';
    auto ext = req[http::field::sec_websocket_extensions];
    std::cout << "Sec-Extension:" << ext << '\n';
    auto p = ext.find("eid=\"");
    if (p != std::string::npos) {
      p += 4;
      auto e = ext.find_last_of("\"", ++p);  // find last double quote
      std::cout << "USP Agnet ID: " << ext.substr(p, e + 1) << '\n';
    }
    */
    // Now send msg to client
    timer_.expires_from_now(std::chrono::seconds(5));
    timer_.async_wait(std::bind(&WebSocketServer::OnWait, this, std::placeholders::_1));
  }

  void OnWait(const boost::system::error_code& ec) {
    char msg[256];
    auto now = std::chrono::system_clock::now();
    std::time_t tt = std::chrono::system_clock::to_time_t(now);
    std::tm tm = *std::localtime(&tt);
    std::stringstream ss;
    ss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
    if (!ec) { 
      PLOG(plog::debug) << "send_ " << ss.str();
      if ( send_ )
        send_( ss.str().c_str(), ss.str().size());
      else
        PLOG(plog::error) << "Send callback function is empty!";
        timer_.expires_from_now(std::chrono::seconds(5));
        timer_.async_wait(std::bind(&WebSocketServer::OnWait, this, std::placeholders::_1));
      return;
    }
    std::cout << "Timer canceled\n";
  }

 public:
  WebSocketServer(net::io_context& ioc, Config& cfg) : ioc_{ioc}, timer_{ioc} {
    // add header options for USP server.
    ws_options_.emplace_back(std::make_pair(http::field::sec_websocket_protocol, &USP_Proto));
    web_socket_ext_ = USP_Ext + '\"' + *cfg.Get("controller-endpoint") + '\"';
    ws_options_.emplace_back(std::make_pair(http::field::sec_websocket_extensions, &web_socket_ext_));

    ws_path_ = *cfg.Get("ws_path");
    PLOG(plog::info) << "ws_path=" << ws_path_;
    bool useSSL = *cfg.Get("encryption") == "true";
    PLOG(plog::info) << "encryption=" << useSSL;

    // Setup the callback and options address.
    auto ws_dope = std::make_shared<asio_ws::WSDope>(
        std::bind(&WebSocketServer::WSPath, this),
        std::bind(&WebSocketServer::msg_handler_func, this, std::placeholders::_1, std::placeholders::_2),
        std::bind(&WebSocketServer::ws_connection_ready, this, std::placeholders::_1),
        std::bind(&WebSocketServer::error_callback, this, std::placeholders::_1, std::placeholders::_2),
        std::bind(&WebSocketServer::GetWSOptions, this));

    std::string host = *cfg.Get("host");
    auto const port = static_cast<unsigned short>(cfg.GetInt("port"));
    PLOG(plog::debug) << "host: " << host << " port: " << port;
    auto const address = net::ip::make_address(host);
    ssl::context ctx{ssl::context::tlsv12};
    server_ = std::make_shared<asio_ws::listener>(
          ioc,
          ctx,
          tcp::endpoint{address, port},
          ws_dope);
    server_->run();
  }

};

int main(int argc, char* argv[]) {
  // Check command line arguments.
  if (argc < 3) {
    std::cerr << "Usage: WSServerTest <config-file> [log-level]\n";
    return EXIT_FAILURE;
  }
  auto const threads = 1; ////std::max<int>(1, std::atoi(argv[3]));
  std::ifstream fin;
  fin.open(argv[1]);
  if (!fin.is_open()) {
    std::cerr << "Unable to open " << argv[1] << '\n';
    return -1;
  }

  enum plog::Severity severity{plog::error};
  if (argc > 2) {
    if (!strcmp(argv[2], "none"))
      severity = plog::none;
    else if (!strcmp(argv[2], "fatal"))
      severity = plog::fatal;
    else if (!strcmp(argv[2], "error"))
      severity = plog::error;
    else if (!strcmp(argv[2], "warning"))
      severity = plog::error;
    else if (!strcmp(argv[2], "info"))
      severity = plog::info;
    else if (!strcmp(argv[2], "debug"))
      severity = plog::debug;
    else if (!strcmp(argv[2], "verbose"))
      severity = plog::verbose;
    else
      severity = plog::error;
  }
  static plog::ConsoleAppender<plog::TxtFormatter> consoleAppender;
  plog::init(severity, &consoleAppender);

  Config cfg{};
  cfg.InitializeConfig(fin);

  // The io_context is required for all I/O
  net::io_context ioc{threads};

  // Capture SIGINT and SIGTERM to perform a clean shutdown
  net::signal_set signals(ioc, SIGINT, SIGTERM);
  signals.async_wait(
      [&](beast::error_code const&, int) {
        // Stop the `io_context`. This will cause `run()`
        // to return immediately, eventually destroying the
        // `io_context` and all of the sockets in it.
        ioc.stop();
      });
  auto wsSockServer = std::make_shared<WebSocketServer>(ioc, cfg);
  // Run the I/O service on the requested number of threads
    std::vector<std::thread> v;
    v.reserve(threads - 1);
    for(auto i = threads - 1; i > 0; --i)
        v.emplace_back(
        [&ioc]
        {
            ioc.run();
        });
    ioc.run();


  // (If we get here, it means we got a SIGINT or SIGTERM)
  PLOG(plog::info) << "Exit WSServerTest";

  return EXIT_SUCCESS;
}