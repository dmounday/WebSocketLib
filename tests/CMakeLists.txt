find_package(OpenSSL REQUIRED)

add_executable (WSClientTest WSClientTest.cpp)
target_link_libraries (WSClientTest PRIVATE
        WebSocketLib
        PUBLIC pthread
        PUBLIC OpenSSL::SSL PUBLIC OpenSSL::Crypto
        )
target_include_directories(WSClientTest PRIVATE 
                              ../src)

                              
add_executable(WSServerTest WSServerTest.cpp Config.cpp)
target_include_directories(WSServerTest PRIVATE ../src)
target_include_directories(WSServerTest INTERFACE ${plog_SOURCE_DIR}/include)
target_link_libraries (WSServerTest PRIVATE
        WebSocketLib
        PUBLIC pthread
        PUBLIC OpenSSL::SSL PUBLIC OpenSSL::Crypto
        )
