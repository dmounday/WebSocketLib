cmake_minimum_required(VERSION 3.18)
project(BeastWebSocketLib LANGUAGES CXX )

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON) 
set(CMAKE_CXX_EXTENSIONS OFF)

include(FetchContent)
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

FetchContent_Declare ( 
  plog
  GIT_REPOSITORY https://github.com/SergiusTheBest/plog.git
  GIT_TAG 1.1.6
  CONFIGURE_COMMAND ""
  BUILD_COMMAND ""
)
FetchContent_MakeAvailable( plog )

set(SRC_FILE_LIST  src/ws_client.cpp
                  src/ws_serverlib.cpp
                  src/ws_client_io.cpp
                  src/ws_ssl_client.cpp
                  src/ws_tcp_client.cpp
)

add_library( WebSocketLib STATIC ${SRC_FILE_LIST})
target_include_directories(WebSocketLib INTERFACE ${plog_SOURCE_DIR}/include)
target_include_directories(WebSocketLib PRIVATE src)
target_link_libraries(WebSocketLib plog )

add_subdirectory(tests)
