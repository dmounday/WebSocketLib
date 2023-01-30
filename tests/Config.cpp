//
// Gatespace Networks Inc. Copyright 2018-2019. All Rights Reserved.
// Gatespace Networks, Inc. Confidential Material.
// Config.cpp
//
//
#include "Config.h"
namespace asio_ws{

void Config::InitializeConfig(std::ifstream& fin) {
  std::string line;
  while (std::getline(fin, line)) {
    if ("//" == line.substr(0, 2) ||
        "#" == line.substr(0, 1))
      continue;  // ignore comment
    auto p = line.find("=");
    if (p != std::string::npos) {
      auto key = line.substr(0, p);
      p = line.find_first_not_of(" =", p);
      if (p != std::string::npos) {
        config[key] = line.substr(p, line.length() - p);  // add to map
        std::cout << key << '=' << config[key] << '\n';
      }
    }
  }
}

const std::string*
Config::Get(const char* key)const {
  auto it = config.find(key);
  if ( it != config.end())
    return &it->second;
  return nullptr;
}
int Config::GetInt(const char* key)const {
  auto p = Get(key);
  if ( p )
    return stoi(*p);
  return 0;
}

}



