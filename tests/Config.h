//
// Gatespace Networks Inc. Copyright 2018-2019. All Rights Reserved.
// Gatespace Networks, Inc. Confidential Material.
// TestConfig.h
//
//  Created on: Feb 21, 2020
//      Author: dmounday
//
#ifndef SRC_TESTCONFIG_H_
#define SRC_TESTCONFIG_H_
#include <map>
#include <vector>
#include <memory>
#include <iostream>
#include <fstream>
#include <sstream>

namespace asio_ws {
class Config {
public:
  inline Config(){};
  void InitializeConfig( std::ifstream& );
  const std::string* Get(const char* key)const;
  int GetInt(const char* key)const;

private:
  std::map<std::string, std::string > config;
};
}
#endif //SRC_TESTCONFIG_H_



