//
// WebSocket client test.
//  1. connect to websocket host and establish session.
//      - Upgrade to WebSocket session
//      - Set USP header and values.
//  2. Send test msg to test server and read reply.
//  3. Continuing reading session messages. 

#include <iostream>
#include <string>
#include <iomanip>
#include <sstream>

#include "boost/asio.hpp"
#include "boost/asio/ssl.hpp"

#include "ws_client.h"

namespace asio_ws {

const std::string USP_Proto{"v1.usp"};
const std::string USP_Ext{"bbf-usp-protocol; eid="};

class ClientUser: std::enable_shared_from_this<ClientUser> {
  std::shared_ptr<WS_Client> client_;
  std::string message_{"Test Message"};
  boost::asio::steady_timer timer_;
  WSOptions options_; 
  int cnt_;
  std::string USP_EP_Ext_;
public:
  ClientUser(boost::asio::io_context& ioc, std::string& agentEndPt,
              const std::string* caFile ):
     timer_{ioc}
  {
    options_.emplace_back(std::make_pair(http::field::sec_websocket_protocol, &USP_Proto));
    USP_EP_Ext_ = USP_Ext + '\"' + agentEndPt + '\"';
    options_.emplace_back(std::make_pair(http::field::sec_websocket_extensions, &USP_EP_Ext_));
    OnMTPReady ready = std::bind(&ClientUser::WSReady, this, std::placeholders::_1);
    ErrorCBFunc failed = std::bind(&ClientUser::WSFailed, this, std::placeholders::_1, std::placeholders::_2);
    MsgHandler handler = std::bind(&ClientUser::MsgHandlerFunc, this, std::placeholders::_1, std::placeholders::_2);
     
    client_ = std::make_shared<WS_Client>(ioc, ready, failed, handler, options_);
    if ( caFile ){
     client_->SetTLS(0, caFile);
    }
    
  }

  void StartClient( std::string& host, std::string& port, std::string& path){
    client_->Start(&host, &port, &path);
  }

  void WSReady(const websocket::response_type& res ){
    std::cout << "response_type: " << '\n';
    for (auto const& field: res)
      std::cout << field.name() << " = " << field.value() << '\n';
    std::cout << "Sec-Protocol: " << res[http::field::sec_websocket_protocol] << '\n';
    auto ext = res[http::field::sec_websocket_extensions];
    std::cout << "Sec-Extension:" << ext << '\n';
    auto p = ext.find("eid=\"");
    if ( p != std::string::npos) {
      p += 4;
      auto e = ext.find_last_of("\"", ++p); // find last double quote
      std::cout << "USP Controller Eid: " << ext.substr(p, e+1 ) << '\n';
    }
    std::cout << "Ready to send messages\n";
    client_->Write(reinterpret_cast<const unsigned char*>(message_.data()), message_.length());  
  };

  void WSFailed(const boost::system::error_code& ec, const std::string& msg){
    std::cout << "Failed: " << ec.message() << ": " << msg << '\n';
  }
  const char* reply{"Client ACK of message from server."};

  void MsgHandlerFunc(const char* msg, std::size_t lth){
    std::cout << "Rcvd Msg lth:" << lth << '>';
    for(auto i=0; i< lth; ++i) std::cout << *(msg+i);
    std::cout << "<\n";
    /* remove to enable client to send messages and wait replies.
    timer_.expires_from_now(std::chrono::seconds(5));
    timer_.async_wait( std::bind( &ClientUser::OnWait,
         this, std::placeholders::_1));
    */

    client_->Write(reinterpret_cast<const unsigned char*>(reply), strlen(reply));
  }

  void OnWait(const boost::system::error_code& ec ){
    char msg[256];
    auto now = std::chrono::system_clock::now();
    std::time_t tt = std::chrono::system_clock::to_time_t(now);
    std::tm tm = *std::localtime(&tt);
    std::stringstream ss;
    ss << std::put_time( &tm, "%Y-%m-%d %H:%M:%S");
    if (!ec ){
      client_->Write( reinterpret_cast<const unsigned char*>(ss.str().c_str()), ss.str().size());
      return;
    }
    std::cout << "Timer canceled\n";
  }
};

} // namespace asio_ws

int main(int argc, char* argv[]){

  if ( argc < 4 ){
    std::cout << "Use: WSClient host port path [caFile-path]\n";
    exit(EXIT_SUCCESS);
  }
  boost::asio::io_context ioc;
  std::string host{argv[1]};
  std::string port{argv[2]};
  std::string path{argv[3]};
  std::string caPath;
  if (argc > 4)
    caPath = std::string{argv[4]};
  std::string endPoint{"self::gs-agent-01"};  
  auto libUser = std::make_shared<asio_ws::ClientUser>(ioc, endPoint, argc>4? &caPath: nullptr);

  libUser->StartClient(host, port, path);
  ioc.run();



}

